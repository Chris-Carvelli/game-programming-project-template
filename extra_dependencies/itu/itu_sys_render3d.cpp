#include <itu_engine.hpp>

#ifndef ITU_UNITY_BUILD
#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#endif // ITU_UNITY_BUILD

#define UNIFORM_SLOT_GLOBAL   0
#define UNIFORM_SLOT_MATERIAL 1

// TMP THESE SHOULD NOT BE HERE!  =============
ITU_IdRawTexture itu_sys_rstorage_raw_texture_load(EngineContext* context, const char* path)  { SDL_assert(false && "NotYetImplemented"); return 0; }
RawTextureData*  itu_sys_rstorage_raw_texture_get_ptr(ITU_IdRawTexture id) { SDL_assert(false && "NotYetImplemented"); return NULL; }
// TMP end ====================================

struct ITU_Material
{
	SDL_GPUShader* shader_base_vert;
	SDL_GPUShader* shader_base_frag;

	SDL_GPUTextureSamplerBinding texture_bindings;

	SDL_GPUTexture* tex_albedo;
	ITU_IdRawTexture tex_albedo_data;
};

struct ITU_Renderer3D
{
	SDL_GPUDevice* device;
	SDL_GPUGraphicsPipeline* pipeline;
	SDL_GPUColorTargetInfo color_target_info;
	SDL_GPUDepthStencilTargetInfo depth_stencil_target_info;
	SDL_GPUTexture* depth_stencil_texture;
	SDL_GPUTexture* color_texture;

	UniformData uniform_data_global;

	SDL_GPUCommandBuffer* command_buffer_upload;
	SDL_GPUCommandBuffer* command_buffer_render;

	// very simple pattern consisting of persistent buffers only. not very performant but simple to implement
	SDL_GPUTransferBuffer* transfer_buffer_vertices;
	SDL_GPUTransferBuffer* transfer_buffer_instances;
	SDL_GPUTransferBuffer* transfer_buffer_indices;
	SDL_GPUTransferBuffer* transfer_buffer_texture;
	SDL_GPUBuffer* buf_data_vertices;
	SDL_GPUBuffer* buf_data_instances;
	SDL_GPUBuffer* buf_data_indices;
	SDL_GPUBuffer* buf_data_texture;
	
	Uint8* base_mapping_data_vertices;
	Uint8* base_mapping_data_instances;
	Uint8* base_mapping_data_indices;
	Uint8* base_mapping_data_texture;

	Uint8* mapping_data_vertices;
	Uint8* mapping_data_instances;
	Uint8* mapping_data_indices;
	Uint8* mapping_data_texture;

	// TMP renderpass
	SDL_GPURenderPass* render_pass;

	// Looks like even telling SDL_GPU that a shader has 2 UBOs, something goes wrong. Need to investigate
	// in the meantime, keep material data in global uniform data (sicne we have a single material anyway)
	//struct TMP_MaterialData
	//{
	//	float k_ambient;
	//	float k_diffuse;
	//	float k_specular;
	//	float exp_specular;
	//};
	// TMP testing material to check 3D rendering capabilities
	//     materials will be resources like everything else, but it requires still a bit of design work
	ITU_Material TMP_material_phong;

	// drawcalls
	DrawCall* drawcalls;
	int drawcalls_count_alive;
	int drawcalls_count_max;
	InstanceData* instance_data;
	int instance_data_count_alive;
	int instance_data_count_max;
};

// private functions
void acquare_basic_resources(ITU_Renderer3D* ctx_rendering, SDL_Window* window, vec2f window_size);
SDL_GPUShader* shader_load(ITU_Renderer3D* ctx_rendering, const char* path, SDL_GPUShaderStage stage, int num_uniform_buffers, int num_samples);
SDL_GPUTexture* gpu_itu_resources_texture_create(SDL_GPUDevice* device, int w, int h, int channels, SDL_GPUTextureUsageFlags usage, SDL_GPUTextureFormat format);
void create_persistent_buffer(ITU_Renderer3D* ctx_rendering, SDL_GPUTransferBuffer* buffer_transfer, Uint32 size, SDL_GPUBuffer** out_buffer, Uint8** out_memory);

void itu_sys_render3d_init(EngineContext* context)
{
	SDL_assert(context->config.type_renderer == TYPE_RENDERER_3D);
	SDL_assert(context->renderer == NULL);

	context->ctx_rendering = (ITU_Renderer3D*)SDL_malloc(sizeof(ITU_Renderer3D));
	ITU_Renderer3D* ctx_rendering = context->ctx_rendering;
	memset(ctx_rendering, 0, sizeof(ITU_Renderer3D));

	// try to use vulkan
	// TODO test on mac, need to be sure that it will fallback to metal if needed!
	ctx_rendering->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
	SDL_ClaimWindowForGPUDevice(ctx_rendering->device, context->window);

	// create temporary material
	{
		ITU_IdRawTexture id_tex = itu_sys_rstorage_raw_texture_load(context, "data/kenney/meshes/Textures/colormap.png");
		RawTextureData* tex_data = itu_sys_rstorage_raw_texture_get_ptr(id_tex);
		ctx_rendering->TMP_material_phong.shader_base_vert = shader_load(ctx_rendering, "data/shader/phong.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX  , 1, 0);
		ctx_rendering->TMP_material_phong.shader_base_frag = shader_load(ctx_rendering, "data/shader/phong.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
		ctx_rendering->TMP_material_phong.tex_albedo = gpu_itu_resources_texture_create(ctx_rendering->device, tex_data->width, tex_data->height, 4, SDL_GPU_TEXTUREUSAGE_SAMPLER, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);
;		ctx_rendering->TMP_material_phong.texture_bindings.texture = ctx_rendering->TMP_material_phong.tex_albedo;
		{
			SDL_GPUSamplerCreateInfo create_info = { };
			ctx_rendering->TMP_material_phong.texture_bindings.sampler = SDL_CreateGPUSampler(ctx_rendering->device, &create_info);
		}
	}

	acquare_basic_resources(ctx_rendering, context->window, { (float)context->config.window_w, (float)context->config.window_h});

	{
		SDL_GPUTransferBufferCreateInfo create_info = { };
		create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		create_info.size = VERTEX_DATA_SIZE_MAX;
		ctx_rendering->transfer_buffer_vertices = SDL_CreateGPUTransferBuffer(ctx_rendering->device, &create_info);
	}
	{
		SDL_GPUTransferBufferCreateInfo create_info = { };
		create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		create_info.size = INSTANCE_DATA_SIZE_MAX;
		ctx_rendering->transfer_buffer_instances = SDL_CreateGPUTransferBuffer(ctx_rendering->device, &create_info);
	}
	{
		SDL_GPUTransferBufferCreateInfo create_info = { };
		create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		create_info.size = INDEX_DATA_SIZE_MAX;
		ctx_rendering->transfer_buffer_indices = SDL_CreateGPUTransferBuffer(ctx_rendering->device, &create_info);
	}
	{
		SDL_GPUTransferBufferCreateInfo create_info = { };
		create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		create_info.size = TEXTURE_DATA_SIZE_MAX;
		ctx_rendering->transfer_buffer_texture = SDL_CreateGPUTransferBuffer(ctx_rendering->device, &create_info);
	}

	create_persistent_buffer(ctx_rendering, ctx_rendering->transfer_buffer_vertices , VERTEX_DATA_SIZE_MAX  , &ctx_rendering->buf_data_vertices , &ctx_rendering->base_mapping_data_vertices);
	create_persistent_buffer(ctx_rendering, ctx_rendering->transfer_buffer_instances, INSTANCE_DATA_SIZE_MAX, &ctx_rendering->buf_data_instances, &ctx_rendering->base_mapping_data_instances);
	create_persistent_buffer(ctx_rendering, ctx_rendering->transfer_buffer_indices  , INDEX_DATA_SIZE_MAX   , &ctx_rendering->buf_data_indices  , &ctx_rendering->base_mapping_data_indices);
	create_persistent_buffer(ctx_rendering, ctx_rendering->transfer_buffer_texture  , TEXTURE_DATA_SIZE_MAX , &ctx_rendering->buf_data_texture  , &ctx_rendering->base_mapping_data_texture);

	ctx_rendering->mapping_data_vertices = ctx_rendering->base_mapping_data_vertices;
	ctx_rendering->mapping_data_indices = ctx_rendering->base_mapping_data_indices;
	ctx_rendering->mapping_data_instances = ctx_rendering->base_mapping_data_instances;
	ctx_rendering->mapping_data_texture = ctx_rendering->base_mapping_data_texture;

	ctx_rendering->drawcalls = (DrawCall*) SDL_calloc(FRAME_DRAWCALL_COUNT_MAX, sizeof(DrawCall));
	ctx_rendering->drawcalls_count_max = FRAME_DRAWCALL_COUNT_MAX;

	ctx_rendering->instance_data = (InstanceData*) SDL_calloc(FRAME_INSTANCE_DATA_COUNT_MAX, sizeof(InstanceData));
	ctx_rendering->instance_data_count_max = FRAME_INSTANCE_DATA_COUNT_MAX;

	// upload texture data
	{
		ctx_rendering->command_buffer_upload = SDL_AcquireGPUCommandBuffer(ctx_rendering->device);
		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(ctx_rendering->command_buffer_upload);

		// TODO this DEFINITELY needs to happen as little as possible
		RawTextureData* tex_data = itu_sys_rstorage_raw_texture_get_ptr(ctx_rendering->TMP_material_phong.tex_albedo_data);
		int texture_data_size = tex_data->width * tex_data->height * 4;
		SDL_memcpy(ctx_rendering->mapping_data_texture, tex_data->texture, texture_data_size);

		SDL_GPUTextureTransferInfo transfer_texture_location = { };
		transfer_texture_location.offset = 0;
		transfer_texture_location.pixels_per_row = tex_data->width;
		transfer_texture_location.rows_per_layer = tex_data->height;
		transfer_texture_location.transfer_buffer = ctx_rendering->transfer_buffer_texture;

		SDL_GPUTextureRegion transfer_texture_destination = { };
		transfer_texture_destination.texture = ctx_rendering->TMP_material_phong.tex_albedo;
		transfer_texture_destination.w = tex_data->width;
		transfer_texture_destination.h = tex_data->height;
		transfer_texture_destination.d = 1;

		static int counter = 0;
		counter++;
		SDL_UploadToGPUTexture(
			copy_pass,
			&transfer_texture_location,
			&transfer_texture_destination,
			true
		);

		SDL_EndGPUCopyPass(copy_pass);
		SDL_VALIDATE_PANIC(SDL_SubmitGPUCommandBuffer(ctx_rendering->command_buffer_upload));
	}

	// TMP uniform data defaults
	{
		ctx_rendering->uniform_data_global.light_direction = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
		ctx_rendering->uniform_data_global.light_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		ctx_rendering->uniform_data_global.light_color_ambient = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
		ctx_rendering->uniform_data_global.k_ambient    =  1.0f;
		ctx_rendering->uniform_data_global.k_diffuse    =  1.0f;
		ctx_rendering->uniform_data_global.k_specular   =  0.0f;
		ctx_rendering->uniform_data_global.exp_specular = 42.0f;
	}
}

Model3D* itu_sys_render3d_load_model3d(ITU_Renderer3D* ctx_rendering, const char* path)
{
	const struct aiScene* scene = aiImportFile(path, aiPostProcessSteps::aiProcess_Triangulate | aiPostProcessSteps::aiProcess_FlipUVs);

	if(!scene)
	{
		SDL_Log("WARNING could not load file %s", path);
		return NULL;
	}
	int submeshes_count = scene->mNumMeshes;
	int vertices_count = 0;
	int indices_count  = 0; 

	for(int i = 0; i < scene->mNumMeshes; ++i)
	{
		struct aiMesh* mesh = scene->mMeshes[i];
		vertices_count += mesh->mNumVertices;
		// NOTE: we are asking assimp to triangulate the mesh after loading it, so we are safe to assume all faces are triangular!
		indices_count += 3 * mesh->mNumFaces;
	}

	size_t size_vertices = sizeof(VertexData) * vertices_count;
	size_t size_indices = sizeof(int) * vertices_count;
	size_t submesh_first_element_array_size = sizeof(int) * vertices_count;
	size_t tota_data_size =
		sizeof(Model3D) +
		size_vertices +
		size_indices + 
		submesh_first_element_array_size;
	Model3D* ret = (Model3D*)SDL_malloc(tota_data_size);
	ret->vertices             = pointer_offset(VertexData, ret, sizeof(Model3D));
	ret->indices              = pointer_offset(int, ret->vertices, size_vertices);
	ret->submesh_first_index  = pointer_offset(int, ret->indices , submesh_first_element_array_size);

	ret->vertices_count = vertices_count;
	ret->indices_count  = indices_count;
	ret->submesh_first_index_count  = submeshes_count;

	int curr_index = 0;
	int curr_vertex = 0;

	// vertives

	// indices
	for(int i = 0; i < scene->mNumMeshes; ++i)
	{
		struct aiMesh* mesh = scene->mMeshes[i];
		const char* name = mesh->mName.data;
		
		// slices
		ret->submesh_first_index[i] = curr_index;

		// vertices
		for(int j = 0; j < mesh->mNumVertices; ++j)
		{
			ret->vertices[curr_vertex].position = value_cast(glm::vec3, mesh->mVertices[j]);
			ret->vertices[curr_vertex].normal   = value_cast(glm::vec3, mesh->mNormals[j]);
			ret->vertices[curr_vertex].uv       = value_cast(glm::vec2, mesh->mTextureCoords[0][j]);
			++curr_vertex;
		}
		
		// indices
		for(int j = 0; j < mesh->mNumFaces; ++j)
		{
			// indices and vertices do not match, assimp will always return us unique vertex set (buf the indices inside the face struct will be wrong)
			ret->indices[curr_index] = curr_index; curr_index++;
			ret->indices[curr_index] = curr_index; curr_index++;
			ret->indices[curr_index] = curr_index; curr_index++;
		}
	}

	// TMP
	SDL_Log("loaded %s", path);
	SDL_Log("\tcount vertices  : %d", ret->vertices_count);
	SDL_Log("\tcount indices   : %d", ret->indices_count);
	SDL_Log("\tcount submeshes : %d", ret->submesh_first_index_count);
	SDL_Log("\tsize vertex data: %llu", sizeof(ret->vertices[0]) * ret->vertices_count);
	SDL_Log("\tsize index  data: %llu", sizeof(ret->indices[0]) * ret->indices_count);
	SDL_Log("\tsize total      : %llu", sizeof(ret));

	// upload to gpu
	{
		SDL_assert(ctx_rendering->mapping_data_vertices + size_vertices < ctx_rendering->base_mapping_data_vertices + VERTEX_DATA_SIZE_MAX);
		SDL_assert(ctx_rendering->mapping_data_indices + size_indices < ctx_rendering->base_mapping_data_indices + INDEX_DATA_SIZE_MAX);

		ctx_rendering->command_buffer_upload = SDL_AcquireGPUCommandBuffer(ctx_rendering->device);
		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(ctx_rendering->command_buffer_upload);

		// upload vertex, index and instance data
		{
			Uint8* ptr_data_vertex = ctx_rendering->mapping_data_vertices;

			ctx_rendering->mapping_data_vertices += size_vertices;

			ret->offset_vertices = ptr_data_vertex - ctx_rendering->base_mapping_data_vertices;

			SDL_memcpy(ptr_data_vertex  , ret->vertices, size_vertices);

			// vertices
			{
				SDL_GPUTransferBufferLocation transfer_buffer_location = { };
				transfer_buffer_location.offset = ret->offset_vertices;
				transfer_buffer_location.transfer_buffer = ctx_rendering->transfer_buffer_vertices;

				SDL_GPUBufferRegion transfer_buffer_destination = { };
				transfer_buffer_destination.buffer = ctx_rendering->buf_data_vertices;
				transfer_buffer_destination.offset = ret->offset_vertices;
				transfer_buffer_destination.size = size_vertices;

				SDL_UploadToGPUBuffer(
					copy_pass,
					&transfer_buffer_location,
					&transfer_buffer_destination,
					false
				);
			}

			// indices
			{
				Uint8* ptr_data_index    = ctx_rendering->mapping_data_indices;

				ctx_rendering->mapping_data_indices += size_indices;

				ret->offset_indices  = ptr_data_index  - ctx_rendering->base_mapping_data_indices;

				SDL_memcpy(ptr_data_index   , ret->indices , size_indices);

				SDL_GPUTransferBufferLocation transfer_buffer_location = { };
				transfer_buffer_location.offset = ret->offset_indices;
				transfer_buffer_location.transfer_buffer = ctx_rendering->transfer_buffer_indices;

				SDL_GPUBufferRegion transfer_buffer_destination = { };
				transfer_buffer_destination.buffer = ctx_rendering->buf_data_indices;
				transfer_buffer_destination.offset = ret->offset_indices;
				transfer_buffer_destination.size = size_indices;

				SDL_UploadToGPUBuffer(
					copy_pass,
					&transfer_buffer_location,
					&transfer_buffer_destination,
					false
				);
			}
		}
		SDL_EndGPUCopyPass(copy_pass);
		SDL_VALIDATE_PANIC(SDL_SubmitGPUCommandBuffer(ctx_rendering->command_buffer_upload));
	}
	return ret;
}

void itu_sys_render3d_frame_begin(ITU_Renderer3D* ctx_rendering)
{
	// TODO can we just store the instance data directly in the mapped transform buffer?
	ctx_rendering->mapping_data_instances = ctx_rendering->base_mapping_data_instances;
	ctx_rendering->drawcalls_count_alive = 0;
	ctx_rendering->instance_data_count_alive = 0;
}

void itu_sys_render3d_set_camera_view(ITU_Renderer3D* ctx_rendering, glm::mat4 view_matrix)
{
	ctx_rendering->uniform_data_global.camera_view     = view_matrix;
	ctx_rendering->uniform_data_global.camera_position = -view_matrix[3];
}
void itu_sys_render3d_set_camera_proj(ITU_Renderer3D* ctx_rendering, glm::mat4 proj_matrix)
{
	ctx_rendering->uniform_data_global.camera_projection = proj_matrix;
}

void itu_sys_render3d_model3d_render(ITU_Renderer3D* ctx_rendering, Model3D* data, InstanceData instance)
{
	if(!data)
		return;

	if(ctx_rendering->drawcalls_count_alive >= FRAME_DRAWCALL_COUNT_MAX)
	{
		SDL_Log("WARNING too many draw calls for this frame!");
		return;
	}

	if(ctx_rendering->instance_data_count_alive >= FRAME_DRAWCALL_COUNT_MAX)
	{
		SDL_Log("WARNING too instances to render for this frame!");
		return;
	}

	int idx = ctx_rendering->drawcalls_count_alive++;
	int instance_first_idx = ctx_rendering->instance_data_count_alive++;
	ctx_rendering->drawcalls[idx].data_mesh = data;
	ctx_rendering->drawcalls[idx].data_instances_count = 1;
	ctx_rendering->drawcalls[idx].data_instances_first_index = instance_first_idx;
	ctx_rendering->instance_data[instance_first_idx] = instance;
}

void itu_sys_render3d_model3d_render_instanced(ITU_Renderer3D* ctx_rendering, Model3D* data, InstanceData* instances, int instances_count)
{
	if(!data)
		return;

	if(!instances)
		return;

	if(ctx_rendering->drawcalls_count_alive >= FRAME_DRAWCALL_COUNT_MAX)
	{
		SDL_Log("WARNING too many draw calls for this frame!");
		return;
	}

	if(ctx_rendering->instance_data_count_alive + instances_count >= FRAME_DRAWCALL_COUNT_MAX)
	{
		SDL_Log("WARNING too instances to render for this frame!");
		return;
	}

	int idx = ctx_rendering->drawcalls_count_alive++;
	int instance_first_idx = ctx_rendering->instance_data_count_alive;
	ctx_rendering->instance_data_count_alive += instances_count;
	ctx_rendering->drawcalls[idx].data_mesh = data;
	ctx_rendering->drawcalls[idx].data_instances_count = instances_count;
	ctx_rendering->drawcalls[idx].data_instances_first_index = instance_first_idx;
	SDL_memcpy(ctx_rendering->instance_data + instance_first_idx, instances, instances_count * sizeof(InstanceData));

}

void itu_sys_render3d_frame_end(EngineContext* context)
{
	ITU_Renderer3D* ctx_rendering = context->ctx_rendering;
	ctx_rendering->command_buffer_render = SDL_AcquireGPUCommandBuffer(ctx_rendering->device);

	SDL_WaitAndAcquireGPUSwapchainTexture(
		ctx_rendering->command_buffer_render,
		context->window,
		&ctx_rendering->color_target_info.texture,
		NULL,
		NULL
	);

	if(ctx_rendering->color_target_info.texture)
	{
		ctx_rendering->render_pass = SDL_BeginGPURenderPass(ctx_rendering->command_buffer_render, &ctx_rendering->color_target_info, 1, &ctx_rendering->depth_stencil_target_info);
	
		SDL_BindGPUGraphicsPipeline(ctx_rendering->render_pass, ctx_rendering->pipeline);
	
		SDL_GPUViewport viewport = { 0 };
		SDL_FRect viewport_rect = itu_lib_camera_get_viewport_rect(context, context->camera_active);
		viewport.x = viewport_rect.x;
		viewport.y = viewport_rect.y;
		viewport.w = viewport_rect.w;
		viewport.h = viewport_rect.h;
		viewport.min_depth = 0;
		viewport.max_depth = 1;
		SDL_SetGPUViewport(ctx_rendering->render_pass, &viewport);

		
		// only once per frame
		{
			ctx_rendering->uniform_data_global.time = context->uptime;
			SDL_PushGPUVertexUniformData(ctx_rendering->command_buffer_render, UNIFORM_SLOT_GLOBAL, &ctx_rendering->uniform_data_global, sizeof(UniformData));
		}
		// every time a material changes (we assume we already sorted materials as best as we can
		//SDL_PushGPUVertexUniformData(ctx_rendering->command_buffer_render, UNIFORM_SLOT_MATERIAL, &ctx_rendering->TMP_material_phong_data, sizeof(TMP_MaterialData));

		for(int i = 0; i < ctx_rendering->drawcalls_count_alive; ++i)
		{
			DrawCall* drawcall = &ctx_rendering->drawcalls[i];

			Uint32 size_instance = sizeof(InstanceData) * drawcall->data_instances_count;
			Uint32 offset_instance;
			// copy pass
			//if(!resources_uploaded)
			{

				SDL_assert(ctx_rendering->mapping_data_instances + size_instance < ctx_rendering->base_mapping_data_instances + INSTANCE_DATA_SIZE_MAX);
				Uint8* ptr_data_instance = ctx_rendering->mapping_data_instances;
				ctx_rendering->mapping_data_instances += size_instance;

				ctx_rendering->command_buffer_upload = SDL_AcquireGPUCommandBuffer(ctx_rendering->device);
				SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(ctx_rendering->command_buffer_upload);

				offset_instance = ptr_data_instance - ctx_rendering->base_mapping_data_instances;
		
				InstanceData* src_instance_data = &ctx_rendering->instance_data[drawcall->data_instances_first_index];
				SDL_memcpy(ptr_data_instance, src_instance_data, size_instance);

				SDL_GPUTransferBufferLocation transfer_buffer_location = { };
				transfer_buffer_location.offset = offset_instance;
				transfer_buffer_location.transfer_buffer = ctx_rendering->transfer_buffer_instances;

				SDL_GPUBufferRegion transfer_buffer_destination = { };
				transfer_buffer_destination.buffer = ctx_rendering->buf_data_instances;
				transfer_buffer_destination.offset = offset_instance;
				transfer_buffer_destination.size = size_instance;

				SDL_UploadToGPUBuffer(
					copy_pass,
					&transfer_buffer_location,
					&transfer_buffer_destination,
					false
				);

				SDL_EndGPUCopyPass(copy_pass);
				SDL_VALIDATE_PANIC(SDL_SubmitGPUCommandBuffer(ctx_rendering->command_buffer_upload));
			}
	

			SDL_GPUBufferBinding bindings_vertex[2] = 
			{
				{ ctx_rendering->buf_data_vertices, drawcall->data_mesh->offset_vertices },
				{ ctx_rendering->buf_data_instances, offset_instance }
			};


			SDL_GPUBufferBinding bindings_indices = { ctx_rendering->buf_data_indices, drawcall->data_mesh->offset_indices };
			SDL_BindGPUVertexBuffers(ctx_rendering->render_pass, 0, bindings_vertex, 2);
			SDL_BindGPUIndexBuffer(ctx_rendering->render_pass, &bindings_indices, SDL_GPU_INDEXELEMENTSIZE_32BIT);
			SDL_BindGPUFragmentSamplers(ctx_rendering->render_pass, 0, &ctx_rendering->TMP_material_phong.texture_bindings, 1);
		
			int num_indices   = drawcall->data_mesh->indices_count;

			SDL_DrawGPUIndexedPrimitives(
				ctx_rendering->render_pass,
				drawcall->data_mesh->indices_count,
				drawcall->data_instances_count,
				0,
				0,
				0
			);
		}

		SDL_EndGPURenderPass(ctx_rendering->render_pass);
	}
	else
	{
		SDL_Log("failed acquiring swapchain texture");
	}
}

void itu_sys_render3d_frame_submit(ITU_Renderer3D* ctx_rendering)
{
	SDL_VALIDATE_PANIC(SDL_SubmitGPUCommandBuffer(ctx_rendering->command_buffer_render));
}


// =====================================================================================
// private functions
// =====================================================================================

void acquare_basic_resources(ITU_Renderer3D* ctx_rendering, SDL_Window* window, vec2f window_size)
{
	// vertex buffer descriptors
	SDL_GPUVertexBufferDescription shader_base_vert_vertex_buffer_descriptions[] =
	{
		{
			1,
			sizeof(InstanceData),
			SDL_GPU_VERTEXINPUTRATE_INSTANCE,
		},
		{
			0,
			sizeof(VertexData),
			SDL_GPU_VERTEXINPUTRATE_VERTEX
		}
	};

	// vertex attribute descriptors
	SDL_GPUVertexAttribute shader_base_vert_vertex_attributes[] = 
	{
		// position
		{
			0,
			0,
			SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			offsetof(VertexData, position)
		},
		// normal
		{
			1,
			0,
			SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			offsetof(VertexData, normal)
		},
		// uv
		{
			2,
			0,
			SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			offsetof(VertexData, uv)
		},
		// instance transform - col 0
		{
			3,
			1,
			SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			offsetof(InstanceData, transform_global) + sizeof(float[4]) * 0
		},
		// instance transform - col 1
		{
			4,
			1,
			SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			offsetof(InstanceData, transform_global) + sizeof(float[4]) * 1
		},
		// instance transform - col 2
		{
			5,
			1,
			SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			offsetof(InstanceData, transform_global) + sizeof(float[4]) * 2
		},
		// instance transform - col 3
		{
			6,
			1,
			SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			offsetof(InstanceData, transform_global)  + sizeof(float[4]) * 3
		},
	};

	ctx_rendering->depth_stencil_texture = gpu_itu_resources_texture_create(
		ctx_rendering->device,
		window_size.x,
		window_size.y,
		1,
		SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		SDL_GPU_TEXTUREFORMAT_D24_UNORM
	);

	SDL_GPUColorTargetDescription color_target_descriptions = { };
	color_target_descriptions.format = SDL_GetGPUSwapchainTextureFormat(ctx_rendering->device, window);
	color_target_descriptions.blend_state.enable_blend = true;
	color_target_descriptions.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
	color_target_descriptions.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
	color_target_descriptions.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	color_target_descriptions.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	color_target_descriptions.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	color_target_descriptions.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;

	SDL_GPUGraphicsPipelineCreateInfo pipeline_def = { };
	pipeline_def.vertex_shader = ctx_rendering->TMP_material_phong.shader_base_vert;
	pipeline_def.fragment_shader = ctx_rendering->TMP_material_phong.shader_base_frag;
	pipeline_def.vertex_input_state.vertex_buffer_descriptions = shader_base_vert_vertex_buffer_descriptions;
	pipeline_def.vertex_input_state.num_vertex_buffers = array_size(shader_base_vert_vertex_buffer_descriptions);
	pipeline_def.vertex_input_state.vertex_attributes = shader_base_vert_vertex_attributes;
	pipeline_def.vertex_input_state.num_vertex_attributes = array_size(shader_base_vert_vertex_attributes);
	pipeline_def.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pipeline_def.depth_stencil_state.enable_depth_test = true;
	pipeline_def.depth_stencil_state.enable_depth_write = true;
	pipeline_def.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
	pipeline_def.target_info = { 0 };
	pipeline_def.target_info.color_target_descriptions = &color_target_descriptions;
	pipeline_def.target_info.num_color_targets = 1;
	pipeline_def.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
	pipeline_def.target_info.has_depth_stencil_target = true;

	ctx_rendering->pipeline = SDL_CreateGPUGraphicsPipeline(ctx_rendering->device, &pipeline_def);
	
	ctx_rendering->depth_stencil_target_info.clear_depth = true;
	ctx_rendering->depth_stencil_target_info.clear_stencil = true;
	ctx_rendering->depth_stencil_target_info.cycle = true;
	ctx_rendering->depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
	ctx_rendering->depth_stencil_target_info.store_op = SDL_GPU_STOREOP_STORE;
	ctx_rendering->depth_stencil_target_info.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
	ctx_rendering->depth_stencil_target_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
	ctx_rendering->depth_stencil_target_info.texture = ctx_rendering->depth_stencil_texture;

	ctx_rendering->color_target_info = { };
	ctx_rendering->color_target_info.clear_color = SDL_FColor{ 0.27f, 0.35f, 0.39f, 0 };
	ctx_rendering->color_target_info.store_op = SDL_GPU_STOREOP_STORE;
	ctx_rendering->color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
	ctx_rendering->color_target_info.cycle = true;
	ctx_rendering->color_target_info.texture = ctx_rendering->color_texture;

}

SDL_GPUShader* shader_load(ITU_Renderer3D* ctx_rendering, const char* path, SDL_GPUShaderStage stage, int num_uniform_buffers, int num_samplers)
{
	size_t shader_code_vert_size;
	void* shader_code_vert = SDL_LoadFile(path, &shader_code_vert_size);
	SDL_GPUShaderCreateInfo shader_base_def = { };
	shader_base_def.code = (Uint8*)shader_code_vert;
	shader_base_def.code_size = shader_code_vert_size;
	shader_base_def.entrypoint = "main";
	shader_base_def.format = SDL_GPU_SHADERFORMAT_SPIRV;
	shader_base_def.stage = stage;
	shader_base_def.num_uniform_buffers = num_uniform_buffers;
	shader_base_def.num_samplers = num_samplers;
	return SDL_CreateGPUShader(ctx_rendering->device, &shader_base_def);
}

SDL_GPUTexture* gpu_itu_resources_texture_create(SDL_GPUDevice* device, int w, int h, int channels, SDL_GPUTextureUsageFlags usage, SDL_GPUTextureFormat format)
{
	SDL_GPUTextureCreateInfo create_info = { };
	create_info.format = format;
	create_info.width = w;
	create_info.height = h;
	create_info.layer_count_or_depth = 1;
	create_info.num_levels = 1;
	create_info.type = SDL_GPU_TEXTURETYPE_2D;
	create_info.usage = usage;

	SDL_GPUTexture* ret;
	SDL_VALIDATE(ret = SDL_CreateGPUTexture(device, &create_info));
	return ret;
}

void create_persistent_buffer(ITU_Renderer3D* ctx_rendering, SDL_GPUTransferBuffer* buffer_transfer, Uint32 size, SDL_GPUBuffer** out_buffer, Uint8** out_memory)
{
	SDL_GPUBufferCreateInfo create_info = { };
	create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX | SDL_GPU_BUFFERUSAGE_INDEX; // TMP FIXME this is not correct, find the correct usage type
	create_info.size = size;
	*out_buffer = SDL_CreateGPUBuffer(ctx_rendering->device, &create_info);
	SDL_VALIDATE(*out_buffer);
	*out_memory = (Uint8*)SDL_MapGPUTransferBuffer(ctx_rendering->device, buffer_transfer, false);
	SDL_VALIDATE(*out_memory);
}

#undef UNIFORM_SLOT_GLOBAL
#undef UNIFORM_SLOT_MATERIAL

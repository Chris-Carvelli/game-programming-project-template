#ifndef ITU_SYS_RENDER_3D_HPP
#define ITU_SYS_RENDER_3D_HPP

#define FRAME_DRAWCALL_COUNT_MAX 1024
#define FRAME_INSTANCE_DATA_COUNT_MAX 2048

#ifndef ITU_UNITY_BUILD
#include <itu_lib_context.hpp>
// #include <itu_resource_storage.hpp>
#include <glm/glm.hpp>
#include <SDL3/SDL_stdinc.h>
#endif //ITU_UNITY_BUILD

// TMP THESE SHOULD NOT BE HERE!  =============
typedef Uint64 ITU_IdModel3D;
typedef Uint64 ITU_IdRawTexture;
struct RawTextureData
{
	void* texture;
	int width;
	int height;
};

ITU_IdRawTexture itu_sys_rstorage_raw_texture_load(EngineContext* context, const char* path);
RawTextureData*  itu_sys_rstorage_raw_texture_get_ptr(ITU_IdRawTexture id);

// TMP end ====================================

struct ITU_Renderer3D;

struct VertexData
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct InstanceData
{
	glm::mat4 transform_global;
};

struct UniformData
{
	// camera
	glm::mat4 camera_view;
	glm::mat4 camera_projection;
	glm::vec3 camera_position; // we don't want to compute the position in the shader, it's the same for every pixel
	float padding0;
	// light
	glm::vec4 light_direction; // w = 1 is positional, w = 0 is directional;
	glm::vec4 light_color;
	glm::vec4 light_color_ambient;

	// misc
	float time;

	// TMP material data
	float k_ambient;
	float k_diffuse;
	float k_specular;
	float exp_specular;
};

// resource data for a mesh
// TODO extract gpu data (as clesely related as they are, they should't be mixed with reosurce data)
struct Model3D
{
	int vertices_count;
	int indices_count;
	int submesh_first_index_count;

	VertexData* vertices;
	int*        indices;
	int*        submesh_first_index;  // index of the first index of the i-th submesh (in the `indices` array above)

	// gpu
	Uint32 offset_vertices;
	Uint32 offset_indices;
};

// runtime data for a mesh + other data needed for rendering (like material and stuff)
struct MeshComponent
{
	ITU_IdModel3D id_model3d;
	// ITU_IdRenderingMaterial id_material;
};

enum CameraType
{
	CAMERA_TYPE_ORTHOGRAPHIC,
	CAMERA_TYPE_PERSPECTIVE,
	CAMERA_TYPE_COUNT,
};

const char* const camera_type_names[] = { "Orthographic", "Perspective", "INVALID" };

struct CameraComponent
{
	CameraType type;
	float near;
	float far;
	// we want two different values, so switching on the fly is less jarring
	float fow;  // for perspective
	float zoom; // for orthographic
};

struct DrawCall
{
	Model3D* data_mesh;
	int data_instances_count;
	// TMP store instance data directly here
	// FIXME dump this into a scratch arena
	int data_instances_first_index;
};

// enough space for 32768 verties (pos, normal, uvs)
#define VERTEX_DATA_SIZE_MAX   KB(256)

// enough space for 131072
#define INDEX_DATA_SIZE_MAX    KB(256)

// enough space for 4096 transforms
#define INSTANCE_DATA_SIZE_MAX KB(256)

// enough for... anything? (TBH probably too much but whatever)
#define UNIFORM_DATA_SIZE_MAX  KB(1)

// enough space for 4 texture with
// - 4k size
// - 4 channels
// - 1 byte per channel
#define TEXTURE_DATA_SIZE_MAX  MB(128)

void itu_sys_render3d_init(EngineContext* context);
Model3D* itu_sys_render3d_load_model3d(ITU_Renderer3D* ctx_rendering, const char* path);

void itu_sys_render3d_frame_begin(ITU_Renderer3D* ctx_rendering);
void itu_sys_render3d_set_camera_view(ITU_Renderer3D* ctx_rendering, glm::mat4 view_matrix);
void itu_sys_render3d_set_camera_proj(ITU_Renderer3D* ctx_rendering, glm::mat4 proj_matrix);
void itu_sys_render3d_model3d_render(ITU_Renderer3D* ctx_rendering, Model3D* data, InstanceData instance);
void itu_sys_render3d_model3d_render_instanced(ITU_Renderer3D* ctx_rendering, Model3D* data, InstanceData* instances, int instances_count);
void itu_sys_render3d_frame_end(EngineContext* context);   // NOTE this is the only call that actually needs
void itu_sys_render3d_frame_submit(ITU_Renderer3D* ctx_rendering);

#endif // ITU_SYS_RENDER_3D_HPP

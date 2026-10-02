#include <itu_engine.hpp>

#ifndef ITU_UNITY_BUILD
#include <stb_ds.h>
#include <itu_common.hpp>
#include <itu_lib_context.hpp>
#include <itu_sys_storage_resource.hpp>
#endif // ITU_UNITY_BUILD

static SDL_Texture* TMP_tex = (SDL_Texture*)0x4242;

const char* itu_sys_resource_names[]
{
	"texture",
	"texture_raw",
	"font",
	"audio",
	"model_3d"
};

const Uint64 itu_sys_resource_sizes[]
{
	sizeof(SDL_Texture*),
	0,
	sizeof(TTF_Font*),
	sizeof(MIX_Audio*),
	0
};

const int ITU_SYS_STORAGE_RESOURCE_REALLOC_SIZE = 16;

struct ITU_ResourceStorageData
{
	void*  resources;
	Uint64 resources_count;
	Uint64 resources_max;
	stbds_hm(ITU_ResourceId, Uint64) hm_id2loc;
	Uint64 resource_size;
	ITU_ResourceId id_next;

	// debug
	const char* debug_resource_name;
};

struct ITU_ResourceStorageContext
{
	ITU_ResourceStorageData storages[ITU_RESOURCE_TYPE_COUNT];
};

// FIXME static context... (should this be in the anctual engine context?)
static ITU_ResourceStorageContext ctx_storage_resource;

void itu_sys_storage_resource_init()
{
	for(int i = 0; i < ITU_RESOURCE_TYPE_COUNT; ++i)
	{
		ctx_storage_resource.storages[i].resources = SDL_malloc(itu_sys_resource_sizes[i] * ITU_SYS_STORAGE_RESOURCE_REALLOC_SIZE);
		ctx_storage_resource.storages[i].resources_count = 0;
		ctx_storage_resource.storages[i].resources_max = ITU_SYS_STORAGE_RESOURCE_REALLOC_SIZE;
		ctx_storage_resource.storages[i].id_next = 1;
		ctx_storage_resource.storages[i].resource_size = itu_sys_resource_sizes[i];
		ctx_storage_resource.storages[i].debug_resource_name = itu_sys_resource_names[i];
	}
}

ITU_ResourceId itu_sys_storage_resource_add(ITU_ResourceStorageData* storage, void* resource_data, const char* debug_name);
bool           itu_sys_storage_resource_get(ITU_ResourceStorageData* storage, ITU_ResourceId id, void** out_resource);
bool           itu_sys_storage_resource_delete(ITU_ResourceStorageData* storage, ITU_ResourceId id);
void           itu_sys_storage_resource_set_debug_name(ITU_ResourceStorageData* storage, ITU_ResourceId id, const char* debug_name);
const char*    itu_sys_storage_resource_get_debug_name(ITU_ResourceStorageData* storage, ITU_ResourceId id);

ITU_ResourceId itu_sys_storage_resource_add(ITU_ResourceStorageData* storage, void* resource_data, const char* debug_name)
{
	Uint64 loc = storage->resources_count;
	ITU_ResourceId ret = storage->id_next++;

	if(loc == storage->resources_max)
	{
		storage->resources_max += ITU_SYS_STORAGE_RESOURCE_REALLOC_SIZE;
		storage->resources = SDL_realloc(
			storage->resources,
			storage->resources_max * storage->resource_size
		);
	}

	void* dst = pointer_offset(void*, storage->resources, loc * storage->resource_size);
	memcpy(dst, resource_data, storage->resource_size);

	storage->resources_count++;
	stbds_hmput(storage->hm_id2loc, ret, loc);

	return ret;
}

bool itu_sys_storage_resource_get(ITU_ResourceStorageData* storage, ITU_ResourceId id, void** out_resource)
{
	// FIXME this loc is probably  wrong, it's the location of the 
	int loc = stbds_hmgeti(storage->hm_id2loc, id);

	// int TMP0 = stbds_hmget(storage->hm_id2loc, id);
	// int TMP1 = storage->hm_id2loc[loc].value;

	if(loc == -1)
		return false;

	void* ptr = pointer_offset(void*, storage->resources, loc * storage->resource_size);
	memcpy(out_resource, ptr, storage->resource_size);

	return true;
}

bool itu_sys_storage_resource_delete(ITU_ResourceStorageData* storage, ITU_ResourceId id)
{
	SDL_assert(false && "TODO NotYetImplemented");

	return true;
}

void itu_sys_storage_resource_set_debug_name(ITU_ResourceStorageData* storage, ITU_ResourceId id, const char* debug_name)
{
	// storage->debug_resource_name[]
}

const char* itu_sys_storage_resource_get_debug_name(ITU_ResourceStorageData* storage, ITU_ResourceId id)
{
	return NULL;
}


ITU_IdTexture itu_sys_storage_resource_texture_load(EngineContext* context, const char* path, SDL_ScaleMode mode)
{
	ITU_IdTexture new_tex_idx = { 0 };
	SDL_Texture*  new_tex = itu_resources_texture_create(context, path, mode);
	TMP_tex = new_tex;

	if(!new_tex)
	{
		SDL_Log("Invalid or not supported texture file '%s'", path);
		return new_tex_idx;
	}

	new_tex_idx.id = itu_sys_storage_resource_add(
		&ctx_storage_resource.storages[ITU_RESOURCE_TYPE_TEXTURE],
		&new_tex,
		path
	);


#ifdef ENABLE_DIAGNOSTICS
	itu_sys_storage_resource_set_debug_name(&ctx_storage_resource.storages[ITU_RESOURCE_TYPE_TEXTURE], new_tex_idx.id, path);
#endif

	return new_tex_idx;
}

SDL_Texture* itu_sys_storage_resource_texture_get(ITU_IdTexture texture_id)
{
	SDL_Texture* ret = NULL;

	itu_sys_storage_resource_get(&ctx_storage_resource.storages[ITU_RESOURCE_TYPE_TEXTURE], texture_id.id, (void**)&ret);

	return ret;
	
}


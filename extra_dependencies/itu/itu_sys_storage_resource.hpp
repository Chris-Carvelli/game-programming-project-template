#ifndef ITU_SYS_STORAGE_RESOURCE_HPP
#define ITU_SYS_STORAGE_RESOURCE_HPP

#ifndef ITU_UNITY_BUILD
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#endif // ITU_UNITY_BUILD

// forward declarations
struct EngineContext;

enum ITU_ResourceType
{
	ITU_RESOURCE_TYPE_TEXTURE,
	ITU_RESOURCE_TYPE_TEXTURE_RAW,
	ITU_RESOURCE_TYPE_FONT,
	ITU_RESOURCE_TYPE_AUDIO,
	ITU_RESOURCE_TYPE_MODEL_3D,
	ITU_RESOURCE_TYPE_COUNT
};

typedef Uint32 ITU_ResourceId;
struct ITU_IdTexture { ITU_ResourceId id; ITU_ResourceType type; };
struct ITU_IdFont    { ITU_ResourceId id; ITU_ResourceType type; };

struct ITU_ResourceStorageContext;



void itu_sys_storage_resource_init();

// TODO make meta-version of these (macros? templates? both?)
ITU_IdTexture itu_sys_storage_resource_texture_load(EngineContext* context, const char* path, SDL_ScaleMode mode);
SDL_Texture*  itu_sys_storage_resource_texture_get(ITU_IdTexture id);


#endif // ITU_SYS_STORAGE_RESOURCE_HPP

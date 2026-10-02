#ifndef ITU_SYS_ECS_HPP
#define ITU_SYS_ECS_HPP

// TODO:
// - [ ] probably lock/unlock is not needed anymore, should get rid of it
// - [x] implement basic functions
// - [x] test thoroughly
//   - [x] fix creation/destruction of entities inside systems
//   - [x] fix ComponentMask vs ComponentId problem
//         => using ComponentMask only, added a map(mask => index in component_pool array)
//            this is not ideal, but it allow sus to support enabling/disabling of entire
//            components at runtime
//   - [x] make order of added components independent from order of registration order
// - [ ] use in E05
// - [ ] solve ES05 multiple files problem
// - [ ] revisit tags
// - [-] implement debug UI
// - [x] address TODOs in the cpp file
// - [x] change all void functions to return bool, SDL style
// - [x] uniform all error handling in all functions (utilizing bool return and context.str_last_error)
// - [ ] list all components on an Entity
// - [ ] add ability to register a "default component data"
// - [ ] add ability to enable/disable systems
// - [ ] add ability reorder systems
// - [ ] add type checks
//   - [ ] on USE_AS_COMPONENT (does this struct exist?)
//   - [ ] on `entity_component_set()` (is the data we are passing of the correc type?)
//
// NOTES:
// - debug names are not copied, need to tell the user somehow!
// - ctx_estorage.entities grows dynamically, but space for component data is pre-allocated to
//   `ENTITIES_COUNT_MAX`
// - why components are assigned an ID at compile time, while systems are all good? Is it because
//   components have a lot of data?
// - `add_system` can be a function. Since it's client-facing, it should get a list of components
//    and tags and build the mask itself (how to do that with types tho?)
//     => `register_component` should define an enum-style global const `ComponentMask_Transform2D`
//
// DIFFS:
// - `itu_sys_ecs_init` doesn't have an option to "enable default systems anymore"
//   systems hsould be enabled when needed. We can also add a `itu_sys_ecs_default_systems_enable()`
//

#include <SDL3/SDL.h>
#include <itu_lib_context.hpp>

const Uint64 ITU_SYS_ECS_ERROR_UNKNOWN_COMPONENT = -1;

// NOTE: this is decided by the size of the `component_mask` type (Uint64).
//       DO NOT CHANGE THIS unless you also increase the size of the bitmask!
const int COMPONENTS_COUNT_MAX    = 64;
// NOTE: this is decided by the size of the `tag_mask` tupe (Uint64).
//       DO NOT CHANGE THIS unless you also increase the size of the bitmask!
const int TAGS_COUNT_MAX          = 64;

const int SYSTEMS_COUNT_MAX       = 64;

const int SYSTEM_COMPONENTS_MAX   = 8;
const int SYSTEM_TAGS_MAX         = 8;

enum ITU_SystemQueue
{
    ITU_SYSTEM_QUEUE_UPDATE,
    ITU_SYSTEM_QUEUE_PHYSICS_PRE,
    ITU_SYSTEM_QUEUE_PHYSICS_POST,
    ITU_SYSTEM_QUEUE_MAX
};

const Uint32 ITU_ENTITY_ID_INVALID_GENERATION = 0;
// unique identifier for an entity. This sould be treated as an opaque handle
struct ITU_EntityId
{
    Uint32 generation;
    Uint32 index;
};

const ITU_EntityId ITU_ENTITY_ID_NULL = ITU_EntityId { ITU_ENTITY_ID_INVALID_GENERATION, 0 };

typedef Uint64 ITU_ComponentType;
typedef Uint64 ITU_TagType;


static_assert(COMPONENTS_COUNT_MAX <= sizeof(ITU_ComponentType) * 8, "COMPONENTS_COUNT_MAX must be smaller or equal than the number of bits in `ITU_MaskComponent`");
static_assert(TAGS_COUNT_MAX <= sizeof(ITU_TagType) * 8, "TAGS_COUNT_MAX must be smaller or equal than the number of bits in `ITU_MaskTag`");

// signature for a system-like update function
typedef void (*ITU_FN_SystemUpdate)(EngineContext* context, ITU_EntityId* entity_ids, Uint64 entity_ids_count);

// signature for a component debug UI render function
typedef void (*ITU_FN_ComponendDebugRender)(EngineContext* context, void* data);

// signature for a component's to_string function
typedef Uint64 (*ITU_FN_ComponendToString)(void* data, char* buf, Uint64 buf_size);

struct ITU_ECSSystemDef
{
    ITU_FN_SystemUpdate fn_update;
    Uint64              mask_components;
    Uint64              mask_tags;
    const char*         debug_name;
};

           void        itu_sys_ecs_init(int starting_entities_count);
           void        itu_sys_ecs_destroy();
           bool        itu_sys_ecs_is_initialized();
           bool        itu_sys_ecs_is_locked();
           void        itu_sys_ecs_lock();
           void        itu_sys_ecs_unlock();
/* done */ void        itu_sys_ecs_reset();
           void        itu_sys_ecs_update(EngineContext* context, ITU_SystemQueue queue);
           const char* itu_sys_ecs_last_error_get();
/* TODO */ void        itu_sys_ecs_entities_destroy_all();

// never used?
// void itu_sys_ecs_component_sort_data(ITU_ComponentType component_type, SDL_CompareCallback fn_compare);
// int  itu_sys_ecs_query_entities(Uint64 component_mask, Uint64 tag_mask);
// void itu_sys_ecs_set_systems(ITU_SystemConfig* system_defs, int systems_count);
// void itu_sys_ecs_system_remove();

// entities
           ITU_EntityId itu_sys_ecs_entity_create          (const char* debug_name);
           const char*  itu_sys_ecs_entity_debug_name_get  (ITU_EntityId id);
/* done */ bool         itu_sys_ecs_entity_debug_name_set  (ITU_EntityId id, const char* debug_name);
/* done */ bool         itu_sys_ecs_entity_equals          (ITU_EntityId a, ITU_EntityId b);
           bool         itu_sys_ecs_entity_is_valid        (ITU_EntityId id);
/* done */ bool         itu_sys_ecs_entity_component_set_ex(ITU_EntityId id, ITU_ComponentType type, void* in_data_copy);
/* done */ bool         itu_sys_ecs_entity_component_get_ex(ITU_EntityId id, ITU_ComponentType type, void* out_data_copy);
/* done */ bool         itu_sys_ecs_entity_component_remove_ex(ITU_EntityId id, ITU_ComponentType type);
           void*        itu_sys_ecs_entity_component_data_ptr_ex(ITU_EntityId id, ITU_ComponentType component_type);
           Uint32       itu_sys_ecs_entity_component_to_string_ex(ITU_EntityId id, ITU_ComponentType component_type, char* buffer, Uint64 buffer_size);
/* done */ bool         itu_sys_ecs_entity_tag_add_ex      (ITU_EntityId id, ITU_TagType tag);
/* done */ bool         itu_sys_ecs_entity_tag_remove_ex   (ITU_EntityId id, ITU_TagType tag);
/* done */ bool         itu_sys_ecs_entity_tag_has_ex      (ITU_EntityId id, ITU_TagType tag);
/* done */ bool         itu_sys_ecs_entity_component_has_ex(ITU_EntityId id, ITU_ComponentType type);
/* done */ bool         itu_sys_ecs_entity_destroy         (ITU_EntityId id);
           bool         itu_sys_ecs_entity_id_to_stringid  (ITU_EntityId id, char* buffer, int max_len);

           // type system crimes: we know our entire struct is 64 bits, so we can definitely
           // store it in a void*, without hasmaps or other shenanigans
           void*        itu_sys_ecs_entity_id_to_generic_pointer(ITU_EntityId id);
           // type system crimes: we know our entire struct is 64 bits, so we can definitely
           // store it in a void*, without hasmaps or other shenanigans
           ITU_EntityId itu_sys_ecs_entity_id_form_generic_pointer(void* p);

// tags
bool        itu_sys_ecs_tag_enable_ex     (ITU_TagType tag_type, Uint64 element_count, const char* debug_name);
bool        itu_sys_ecs_tag_disable_ex    (ITU_TagType tag_type);
const char* itu_sys_ecs_tag_get_debug_name(ITU_TagType tag_type);

// components
bool        itu_sys_ecs_component_enable_ex         (ITU_ComponentType component_type, Uint64 element_size, Uint64 element_count, const char * debug_name);
bool        itu_sys_ecs_component_disable_ex        (ITU_ComponentType component_type);
bool        itu_sys_ecs_component_is_enabled_ex     (ITU_ComponentType component_type);
const char* itu_sys_ecs_component_debug_name_get_ex (ITU_ComponentType component_type);
void        itu_sys_ecs_component_fn_tostring_set_ex(ITU_ComponentType component_type, ITU_FN_ComponendToString fn_tostring);
void        itu_sys_ecs_component_fn_debug_ui_set_ex(ITU_ComponentType component_type, ITU_FN_ComponendDebugRender fn_debug_ui);

// systems
// bool itu_sys_ecs_system_add(ITU_ECSSystemDef* system_def);
bool        itu_sys_ecs_system_add_ex(ITU_FN_SystemUpdate fn_update, ITU_SystemQueue queue, Uint64 mask_components, Uint64 mask_tags, const char* debug_name);
bool        itu_sys_ecs_system_disable(ITU_FN_SystemUpdate system, ITU_SystemQueue queue);
const char* itu_sys_ecs_system_queue_debug_name_get(ITU_SystemQueue queue);
// utility function used when specifying a new component
//
// DO NOT CALL THIS DIRECTLY! Use instead the macro `ITU_ECS_COMPONENT`
//
// example: `ITU_ECS_COMPONENT(Position) { float x, y; };`
ITU_ComponentType get_new_component_type_ex();

// utility function used when specifying a new tag
//
// DO NOT CALL THIS DIRECTLY! Use instead the macro `ITU_ECS_TAG`
//
// example: `ITU_ECS_TAG(MainCamera);`
ITU_TagType get_new_tag_type_ex();

// utility macros (for type safe-ish component manipulation)
#define ITU_ECS_COMPONENT(T) const ITU_ComponentType Component_##T = get_new_component_type_ex(); \
struct T

#define ITU_ECS_USE_AS_COMPONENT(T) const ITU_ComponentType Component_##T = get_new_component_type_ex()

#define ITU_ECS_TAG(name) const ITU_TagType Tag_##name = get_new_tag_type_ex();

#define itu_sys_ecs_entity_component_set(id, T, ptr_data)            itu_sys_ecs_entity_component_set_ex((id), Component_##T, (ptr_data))
#define itu_sys_ecs_entity_component_get(id, T, ptr_data)            itu_sys_ecs_entity_component_get_ex((id), Component_##T, (ptr_data))
#define itu_sys_ecs_entity_component_remove(id, T)                   itu_sys_ecs_entity_component_remove_ex((id), Component_##T)
#define itu_sys_ecs_entity_component_data_ptr(id, T)             (T*)itu_sys_ecs_entity_component_data_ptr_ex((id), Component_##T)
#define itu_sys_ecs_entity_component_to_string(id, T, buf, buf_size) itu_sys_ecs_entity_component_to_string_ex((id), Component_##T, (buf), (bug_size))
#define itu_sys_ecs_entity_component_has(id, T)                      itu_sys_ecs_entity_component_has_ex(id, Component_##T)

#define itu_sys_ecs_entity_tag_add(id, T)     itu_sys_ecs_entity_tag_add_ex(id, Tag_##T)
#define itu_sys_ecs_entity_tag_has(id, T)     itu_sys_ecs_entity_tag_has_ex(id, Tag_##T)
#define itu_sys_ecs_entity_tag_remove(id, T)  itu_sys_ecs_entity_tag_remove_ex(id, Tag_##T)


#define itu_sys_ecs_tag_enable(name, capacity) itu_sys_ecs_tag_enable_ex(Tag_##name, capacity, #name)

#define itu_sys_ecs_component_mask(T) Component_##T
#define itu_sys_ecs_component_enable(T, elements_max) itu_sys_ecs_component_enable_ex(Component_##T, sizeof(T), elements_max, #T)
#define itu_sys_ecs_component_disable(T)              itu_sys_ecs_component_disable_ex(Component_##T)
#define itu_sys_ecs_component_is_enabled(T)           itu_sys_ecs_component_is_enabled_ex(Component_##T)
#define itu_sys_ecs_component_debug_name_get(T)       itu_sys_ecs_component_debug_name_get_ex(Component_##T)
#define itu_sys_ecs_component_fn_tostring_set(T, fn)  itu_sys_ecs_component_fn_tostring_set_ex(Component_##T, (fn))
#define itu_sys_ecs_component_fn_debug_ui_set(T, fn)  itu_sys_ecs_component_fn_debug_ui_set_ex(Component_##T, (fn))

#define itu_sys_ecs_system_add(fn_update, queue, mask_components, mask_tags) itu_sys_ecs_system_add_ex((fn_update), (queue), (mask_components), (mask_tags), #fn_update)


#endif // ITU_SYS_ECS_HPP
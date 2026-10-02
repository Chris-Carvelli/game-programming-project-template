#include <itu_engine.hpp>

#include <itu_sys_ecs_internal.hpp>

// NOTE:
// - public API should ALWAYS check for entity validity, which means internal API shouldn't
//   need to (might change if we start manipulating IDs or stuff, but for now we don't)
//
// TODO interal:
// -  [ ] clear up names for internal methods and structs


// FIXME extern
extern const ITU_ComponentType Component_PhysicsBody;
extern bool itu_sys_ecs_entity_remove_b2body(ITU_EntityId id_entity);

ITU_EntityStorageContext ctx_estorage;

Uint32 ITU_COMPONENT_COUNT = 0;
ITU_ComponentType get_new_component_type_ex()
{
    SDL_assert(ITU_COMPONENT_COUNT < COMPONENTS_COUNT_MAX);
    return 1llu << ITU_COMPONENT_COUNT++;
}

Uint32 ITU_TAG_COUNT = 0;
ITU_TagType get_new_tag_type_ex()
{
    SDL_assert(ITU_TAG_COUNT < TAGS_COUNT_MAX);
    return 1llu << ITU_TAG_COUNT++;
}
// =============================================================================================
// Section: internal methods declaration
// =============================================================================================


Uint64            itu_ecs_component_mask_to_id(ITU_ComponentType type) { return log2(hibit(type)); }
ITU_ComponentType itu_ecs_component_id_to_mask(Uint64 loc)             { return 1llu << loc;       }
Uint64            itu_ecs_tag_mask_to_id(ITU_TagType type)             { return log2(hibit(type)); }
ITU_TagType       itu_ecs_tag_id_to_mask(Uint64 loc)                   { return 1llu << loc;       }

void itu_ecs_entities_do_destruction();
void itu_ecs_entity_do_destruction(ITU_EntityId);

void  itu_ecs_component_pool_assign(ITU_Component* component_pool, ITU_EntityId entity);
void  itu_ecs_component_pool_remove(ITU_Component* component_pool, ITU_EntityId entity);
void* itu_ecs_component_pool_data_get_ptr(ITU_Component* component_pool, ITU_EntityId entity);
void  itu_ecs_component_pool_data_get(ITU_Component* component_pool, ITU_EntityId entity, void* out_data_copy);
void  itu_ecs_component_pool_data_set(ITU_Component* component_pool, ITU_EntityId entity, void* in_data_copy);

void  itu_ecs_tag_pool_assign(ITU_ComponentTagStorage* tag_pool, ITU_EntityId id);
void  itu_ecs_tag_pool_remove(ITU_ComponentTagStorage* tag_pool, ITU_EntityId id);

// sets str` as last error, to be retrieved by the client application
// NOTE this funtion makes a new copy of `str`, so it is safe to deallocate it afterwards
void itu_ecs_set_last_error(const char* str, ...);

// =============================================================================================
// Section: public API implementation
// =============================================================================================

void itu_sys_ecs_init(int starting_entities_count)
{
    ctx_estorage.flags = ITU_ECS_CONTEXT_FLAGS_INITIALIZED;

    // TODO zero everything,
    // allocate a minimum of elements at initialization time, to minimize early reallocs
    stbds_arrsetcap(ctx_estorage.entities, starting_entities_count);
    stbds_arrsetcap(ctx_estorage.entities_free, STARTING_FREE_ENTITIES_COUNT);
    stbds_arrsetcap(ctx_estorage.entities_system_filtered, starting_entities_count);
    
    ctx_estorage.debug_names_entity = (char*)SDL_calloc(starting_entities_count, STR_ENTITY_DEBUG_NAME_SIZE);

    ctx_estorage.str_last_error = (char*)SDL_malloc(STR_LAST_ERROR_SIZE);
    itu_ecs_set_last_error("No errors yet");


    // TMP setup tag count
    ctx_estorage.tags_count = ITU_TAG_COUNT;
    SDL_memset(ctx_estorage.tags, 0, ctx_estorage.tags_count);
}

void itu_sys_ecs_destroy()
{
    SDL_free(ctx_estorage.str_last_error);
    SDL_free(ctx_estorage.debug_names_entity);
    stbds_arrfreef(ctx_estorage.entities_system_filtered);
    stbds_arrfreef(ctx_estorage.entities_free);
    stbds_arrfreef(ctx_estorage.entities);
    stbds_hmfree(ctx_estorage.fn_component_debug_ui_render);
    stbds_hmfree(ctx_estorage.fn_component_to_string);
    stbds_hmfree(ctx_estorage.index_components);

    ctx_estorage.flags = ITU_ECS_CONTEXT_FLAGS_NONE;
}

bool itu_sys_ecs_is_initialized()
{
    return ctx_estorage.flags & ITU_ECS_CONTEXT_FLAGS_INITIALIZED;
}

bool itu_sys_ecs_is_locked()
{
    return ctx_estorage.flags & ITU_ECS_CONTEXT_FLAGS_UPDATING || ctx_estorage.flags & ITU_ECS_CONTEXT_FLAGS_LOCKED;
}

void itu_sys_ecs_lock()
{
    ctx_estorage.flags |= ITU_ECS_CONTEXT_FLAGS_LOCKED;
}

void itu_sys_ecs_unlock()
{
    // FIXME temporary  doing the cleanup here
    itu_ecs_entities_do_destruction();

    ctx_estorage.flags &= ~ITU_ECS_CONTEXT_FLAGS_LOCKED;
}

void itu_sys_ecs_reset()
{
    // TODO do we want to reset entity generation?
    stbds_arrsetlen(ctx_estorage.entities, 0);
    stbds_arrsetlen(ctx_estorage.entities_free, 0);
    stbds_arrsetlen(ctx_estorage.entities_system_filtered, 0);

    for(Uint32 i = 0; i < stbds_hmlen(ctx_estorage.index_components); ++i)
    {
        ITU_Component* component = ctx_estorage.index_components[i].value;
        component->count_alive = 0;
        SDL_memset(component->data_loc, -1, sizeof(Uint64) * component->count_max);
    }

    for(Uint32 i = 0; i < ctx_estorage.tags_count; ++i)
    {
        ITU_ComponentTagStorage* tag = ctx_estorage.tags[i];
        // FIXME    
        if(!tag)
            continue;
        tag->count_alive = 0;
        SDL_memset(tag->loc, -1, sizeof(Uint64) * tag->count_max);
    }

    // TODO do we need to reset debug names?
}

void itu_sys_ecs_update(EngineContext* context, ITU_SystemQueue queue)
{
    ctx_estorage.flags |= ITU_ECS_CONTEXT_FLAGS_UPDATING;

    for(Uint32 i = 0; i < ctx_estorage.systems_count[queue]; ++i)
    {
        ITU_System* system = &ctx_estorage.systems[queue][i];

        int final_ids_count = itu_ecs_system_get_matching_entities(system, ctx_estorage.entities_system_filtered);

        // if(final_ids_count > 0)
            system->fn_update(context, ctx_estorage.entities_system_filtered, final_ids_count);
    }

    ctx_estorage.flags &= ~ITU_ECS_CONTEXT_FLAGS_UPDATING;
}

const char* itu_sys_ecs_last_error_get()
{
    return ctx_estorage.str_last_error;
}

ITU_EntityId itu_sys_ecs_entity_create(const char* debug_name)
{
    // FIXME maybe creating entitites while locked is actually fine?
    // if(itu_sys_ecs_is_locked())
    // {
    //     itu_ecs_set_last_error("cannot create entity while updating");

    //     // TODO add command queues

    //     return ITU_ENTITY_ID_NULL;
    // }

    if(stbds_arrlen(ctx_estorage.entities_free) > 0)
    {
        ITU_EntityId id_recycled = stbds_arrpop(ctx_estorage.entities_free);
        ITU_Entity*  entity_recicled = &ctx_estorage.entities[id_recycled.index];
        // superfluous, we should hve done this already the first time we used this entity
        // ctx_estorage.entities[id_recycled.index].id.index = id_recycled.index;
        entity_recicled->id.generation = id_recycled.generation + 1;
        entity_recicled->component_mask = 0;
        itu_sys_ecs_entity_debug_name_set(entity_recicled->id, debug_name);
        return entity_recicled->id;
    }

    // Uint64 len = stbds_arrlen(ctx_estorage.entities);
    // Uint64 cap = stbds_arrcap(ctx_estorage.entities);

    // if(len == cap)
    // {
    //     // resize arrays. We need to keep them aligned, so we take care of the resizing
    //     // ourselves (instead of letting )
    // }
    ITU_Entity entity_data;
    entity_data.id.index = stbds_arrlen(ctx_estorage.entities);
    entity_data.id.generation = 1;
    entity_data.component_mask = 0;
    entity_data.tag_mask = 0;

    stbds_arrput(ctx_estorage.entities, entity_data);

    itu_sys_ecs_entity_debug_name_set(entity_data.id, debug_name);

    // FIXME terrible! We are ensuring that our auxiliary array is has the capacity to contain
    //       all the entities. Another point for doing paged system updates
    Uint32 cap = stbds_arrcap(ctx_estorage.entities);
    if(cap > stbds_arrlen(ctx_estorage.entities_system_filtered))
        stbds_arrsetcap(ctx_estorage.entities_system_filtered, cap);

    return entity_data.id;
}

const char* itu_sys_ecs_entity_debug_name_get(ITU_EntityId id)
{
    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity id");
        return NULL;
    }

    ITU_EntityId entity_id = ctx_estorage.entities[id.index].id;
    if(entity_id.generation != id.generation)
    {
        itu_ecs_set_last_error("generation mismatch. Supplied: %u, actual: %u", id.generation, entity_id.generation);
        return NULL;
    }

    return &ctx_estorage.debug_names_entity[id.index * STR_ENTITY_DEBUG_NAME_SIZE];
}

bool itu_sys_ecs_entity_debug_name_set(ITU_EntityId id, const char* debug_name)
{
    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity id");
        return false;
    }

    ITU_EntityId entity_id = ctx_estorage.entities[id.index].id;
    if(entity_id.generation != id.generation)
    {
        itu_ecs_set_last_error("generation mismatch. Supplied: %u, actual: %u", id.generation, entity_id.generation);
        return false;
    }

    char* dst = ctx_estorage.debug_names_entity + STR_ENTITY_DEBUG_NAME_SIZE * id.index;
    if(!debug_name)
    {
        // if we got no debug name, set it to empty stirng
        *dst = 0;
        return true;
    }
    SDL_strlcpy(dst, debug_name, STR_ENTITY_DEBUG_NAME_SIZE);

    return true;
}

bool itu_sys_ecs_entity_equals(ITU_EntityId a, ITU_EntityId b)
{
    return a.generation == b.generation && a.index == b.index;
}

bool itu_sys_ecs_entity_is_valid(ITU_EntityId id)
{
    if(id.index >= stbds_arrlen(ctx_estorage.entities))
        return false;

    // NOTE testing `id.index != -1` is superfluous as long `index` is unsigned
    // NOTE above might not hold if we use splecific bit sizes for generation and index
    return id.generation != ITU_ENTITY_ID_INVALID_GENERATION && ctx_estorage.entities[id.index].id.generation == id.generation;
}

bool itu_sys_ecs_entity_component_set_ex(ITU_EntityId id, ITU_ComponentType type, void* in_data_copy)
{
    ITU_Component* component = stbds_hmget(ctx_estorage.index_components, type);
    if(!component)
    {
        itu_ecs_set_last_error("Unrecognized component. Did you forget to call enable_component()?");
        return false;
    }

    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity id");
        return false;
    }

    bool has_component = ctx_estorage.entities[id.index].component_mask & type;

    // NOTE this branching is terrible. Maybe having two different set and add is not that bad
    //      after all...
    if(!has_component)
    {
        // FIXME maybe adding components while locked is fine?
        // if(itu_sys_ecs_is_locked())
        // {
        //     itu_ecs_set_last_error("cannot add component to entity while updating");

        //     // TODO add command queues

        //     return false;
        // }

        if(component->count_alive >= component->count_max)
        {
            itu_ecs_set_last_error("component data full (max count: %d)", component->count_max);
            return false;
        }

        // add new component to entity's component mask
        ctx_estorage.entities[id.index].component_mask |= type;
        itu_ecs_component_pool_assign(component, id);
    }

    if(!in_data_copy)
    {
        if(!has_component)
            itu_ecs_set_last_error( "no data to set received");
        return false;
    }

    itu_ecs_component_pool_data_set(component, id, in_data_copy);

    return true;
}

bool itu_sys_ecs_entity_component_get_ex(ITU_EntityId id, ITU_ComponentType type, void* out_data_copy)
{
    ITU_Component* component = stbds_hmget(ctx_estorage.index_components, type);
    if(!component)
    {
        itu_ecs_set_last_error("unrecognized component. Did you forget to call enable_component()?");
        return false;
    }

    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity id");
        return false;
    }

    if(!(ctx_estorage.entities[id.index].component_mask & type))
    {
        itu_ecs_set_last_error("entity %d doesn't have component type %lu\n", id.index, type);
        return false;
    }

    if(!out_data_copy)
    {
        itu_ecs_set_last_error("no out buffer received");
        return false;
    }

    itu_ecs_component_pool_data_get(component, id, out_data_copy);

    return true;
}

bool itu_sys_ecs_entity_component_remove_ex(ITU_EntityId id, ITU_ComponentType type)
{
    if(itu_sys_ecs_is_locked())
    {
        itu_ecs_set_last_error("cannot remove component from entity while updating");

        // TODO add command queues

        return false;
    }

    ITU_Component* component = stbds_hmget(ctx_estorage.index_components, type);
    if(!component)
    {
        itu_ecs_set_last_error("unrecognized component. Did you forget to call enable_component()?");
        return false;
    }

    if(!(ctx_estorage.entities[id.index].component_mask & type))
    {
        itu_ecs_set_last_error("entity %d alread doesn't have component type %lu\n", id.index, type);
        return false;
    }

    ctx_estorage.entities[id.index].component_mask ^= type;

    itu_ecs_component_pool_remove(component, id);

    return true;
}

void* itu_sys_ecs_entity_component_data_ptr_ex(ITU_EntityId id, ITU_ComponentType type)
{
    // FIXME ideally we would want to limit the use of this function form the user as much as
    //       possible.
    // TODO make two version of this function:
    //      - const void* get_data_pointer_readonly: always available
    //      - void*       get_data_pointer         : only within systems

    ITU_Component* component = stbds_hmget(ctx_estorage.index_components, type);
    if(!component)
    {
        itu_ecs_set_last_error("unrecognized component. Did you forget to call enable_component()?");
        return NULL;
    }

    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity\n");
        return NULL;
    }

    if(!(ctx_estorage.entities[id.index].component_mask & type))
    {
        itu_ecs_set_last_error("entity %d does NOT have component type %d\n", id.index, type);
        return NULL;
    }

    return itu_ecs_component_pool_data_get_ptr(component, id);
}

Uint32 itu_sys_ecs_entity_component_to_string_ex(ITU_EntityId id, ITU_ComponentType component_type, char* buffer, Uint64 buffer_size)
{
    ITU_Component* component = stbds_hmget(ctx_estorage.index_components, component_type);
    if(!component)
    {
        itu_ecs_set_last_error("unrecognized component. Did you forget to call enable_component()?");
        return 0;
    }

    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity id");
        return 0;
    }

    if(!(ctx_estorage.entities[id.index].component_mask & component_type))
    {
        itu_ecs_set_last_error("entity %d doesn't have component type %lu\n", id.index, component_type);
        return 0;
    }

    if(!buffer)
    {
        itu_ecs_set_last_error("no out buffer received");
        return 0;
    }

    void* data = itu_sys_ecs_entity_component_data_ptr_ex(id, component_type);

    ITU_FN_ComponendToString fn_to_string = stbds_hmget(ctx_estorage.fn_component_to_string, component_type);

    if(!fn_to_string)
    {
        itu_ecs_set_last_error("to_string function not set for component %lu", component_type);
        return 0;
    }

    return fn_to_string(data, buffer, buffer_size);
}

bool itu_sys_ecs_entity_tag_add_ex(ITU_EntityId id, ITU_TagType type)
{
    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity id");
        return false;
    }

    if(ctx_estorage.entities[id.index].tag_mask & type)
    {
        // entity already has the tag, nothing to do
        // should this be an error?
        itu_ecs_set_last_error("entity already has tag");
        return true;
    }

    // FIXME we need to check if the type has only one bit set, otherwise it will mess up the
    //       entity's tag_mask
    // update component mask
    ctx_estorage.entities[id.index].tag_mask |= type;


    // add entity id to tag storage
    Uint64 loc = itu_ecs_tag_mask_to_id(type);
    itu_ecs_tag_pool_assign(ctx_estorage.tags[loc], id);

    return true;
}

bool itu_sys_ecs_entity_tag_remove_ex(ITU_EntityId id, ITU_TagType type)
{
    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity id");
        return false;
    }

    // FIXME we need to check if the type has only one bit set, otherwise it will mess up the
    //       entity's tag_mask
    if(!(ctx_estorage.entities[id.index].tag_mask & type))
    {
        // entity already doesn't have the tag, nothing to do
        // should this be an error?
        itu_ecs_set_last_error("entity already does not have tag");
        return true;
    }

    // update component mask
    ctx_estorage.entities[id.index].tag_mask &= ~type;

    // remove entity id from tag storage
    Uint64 loc = itu_ecs_tag_mask_to_id(type);
    itu_ecs_tag_pool_remove(ctx_estorage.tags[loc], id);

    return true;
}

bool itu_sys_ecs_entity_tag_has_ex(ITU_EntityId id, ITU_TagType type)
{
    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity id");
        return false;
    }

    return ctx_estorage.entities[id.index].tag_mask & type;
}

bool itu_sys_ecs_entity_component_has_ex(ITU_EntityId id, ITU_ComponentType type)
{
    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity id");
        return false;
    }

    return ctx_estorage.entities[id.index].component_mask & type;
}

bool itu_sys_ecs_entity_destroy(ITU_EntityId id)
{
    if(itu_sys_ecs_is_locked())
    {
        // itu_ecs_set_last_error("cannot destroy entity while updating");

        stbds_arrpush(ctx_estorage.entities_pending_deletion, id);

        return true;
    }

    if(!itu_sys_ecs_entity_is_valid(id))
    {
        itu_ecs_set_last_error("invalid entity [id: %d, gen: %d]", id.index, id.generation);
        return false;
    }

    itu_ecs_entity_do_destruction(id);

    return true;
}

bool itu_sys_ecs_entity_id_to_stringid(ITU_EntityId id, char* buffer, int max_len)
{
    // NOTE: this is super slow, but we have to do this if we want to leverage imgui for out UI toolkit
    int ret = SDL_snprintf(buffer, max_len, "%d-%d", id.generation, id.index);
    if(ret <= 0)
    {
        itu_ecs_set_last_error("error %d while composing tringid (see `SDL_snprintf`) docs for info");
        return false;
    }
    return true;
}

void* itu_sys_ecs_entity_id_to_generic_pointer(ITU_EntityId id)
{
    return (void*)*(Uint64*)&id;
}

ITU_EntityId itu_sys_ecs_entity_id_form_generic_pointer(void* p)
{
    return *(ITU_EntityId*)&p;
}

bool itu_sys_ecs_tag_enable_ex(ITU_TagType tag_type, Uint64 element_count, const char* debug_name)
{
    // FIXME enablind doesn't do anything useful anyway. Better solution is to store debug name
    //       at registration and be done (this way, also components are ok:  enabling/disabling)
    //       still works for them since they need to allocate storage, but everything is aligned)
    if(itu_sys_ecs_is_locked())
    {
        itu_ecs_set_last_error("cannot enable tag while updating");

        // TODO add command queues (component stuff might just need to happen outside systems)

        return false;
    }

    // NOTE this will work even if the mask is not a single bit! It will convert to the leftmost
    //      component. Probably don't want that.
    Uint64 loc = itu_ecs_tag_mask_to_id(tag_type);

    if(ctx_estorage.tags[loc])
    {
        itu_ecs_set_last_error("tag already enabled");
        return false;
    }

    size_t size_metadata   = sizeof(ITU_ComponentTagStorage);
    size_t size_loc        = sizeof(Uint64) * element_count;
    size_t size_entity_ids = sizeof(ITU_EntityId) * element_count;
    size_t total_size = size_metadata + size_loc + size_entity_ids;
    
    ctx_estorage.tags[loc] = (ITU_ComponentTagStorage*)SDL_malloc(total_size);
    SDL_memset(ctx_estorage.tags[loc], -1, total_size);

    ctx_estorage.tags[loc]->count_max   = element_count;
    ctx_estorage.tags[loc]->count_alive = 0;

    ctx_estorage.tags[loc]->loc        = pointer_offset(Uint64, ctx_estorage.tags[loc], size_metadata);
    ctx_estorage.tags[loc]->entity_ids = pointer_offset(ITU_EntityId, ctx_estorage.tags[loc]->loc, size_loc);

    ctx_estorage.debug_names_tag[loc] = debug_name;

    return true;
}

bool itu_sys_ecs_tag_disable_ex(ITU_TagType tag_type)
{
    SDL_assert(false && "TODO NotYetImplemented");
    return false;
}

const char* itu_sys_ecs_tag_get_debug_name(ITU_TagType tag_type)
{
    // NOTE this will work even if the mask is not a single bit! It will convert to the leftmost
    //      component. Probably don't want that.
    Uint64 loc = itu_ecs_tag_mask_to_id(tag_type);
    return ctx_estorage.debug_names_tag[loc];
}


bool itu_sys_ecs_component_enable_ex(ITU_ComponentType component_type, Uint64 element_size, Uint64 element_count, const char * debug_name)
{
    if(itu_sys_ecs_is_locked())
    {
        itu_ecs_set_last_error("cannot enable component while updating");

        // TODO add command queues (component stuff might just need to happen outside systems)

        return false;
    }

    if(stbds_hmget(ctx_estorage.index_components, component_type))
    {
        itu_ecs_set_last_error("component already eabled");
        SDL_assert(false);
    }

    // common pattern: we are allocating enough space for the metadata (ITU_Component) + the array data
    size_t size_metadata   = sizeof(ITU_Component);
    size_t size_data_loc   = sizeof(Uint64) * element_count;
    size_t size_entity_ids = sizeof(ITU_EntityId) * element_count;
    size_t size_data       = element_size * element_count;
    size_t total_size = size_metadata + size_data_loc + size_entity_ids + size_data;

    ITU_Component* pool = (ITU_Component*)SDL_malloc(total_size);
    SDL_memset(pool, -1, total_size);

    pool->type         = component_type;
    pool->element_size = element_size;
    pool->count_max    = element_count;
    pool->count_alive  = 0;

    //Uint64* valid = (Uint64*)((unsigned char*)ret + size_metadata);

    pool->data_loc   = pointer_offset(Uint64, pool, size_metadata);                 // first array starts at the end of the metadata
    pool->entity_ids = pointer_offset(ITU_EntityId, pool->data_loc, size_data_loc); // second array starts at the end of first array
    pool->data       = pointer_offset(void, pool->entity_ids, size_entity_ids);     // third array starts at the end of second array

    stbds_hmput(ctx_estorage.index_components, component_type, pool);
    stbds_hmput(ctx_estorage.debug_names_component, component_type, debug_name);

    return true;
}

bool itu_sys_ecs_component_disable_ex(ITU_ComponentType component_type)
{
    if(itu_sys_ecs_is_locked())
    {
        itu_ecs_set_last_error("cannot disable component while updating");

        // TODO add command queues (component stuff might just need to happen outside systems)

        return false;
    }

    ITU_Component* pool = stbds_hmget(ctx_estorage.index_components, component_type);
    if(!pool)
    {
        itu_ecs_set_last_error("component not enabled");
        SDL_assert(false);
    }

    SDL_free(pool);
    stbds_hmdel(ctx_estorage.fn_component_debug_ui_render, component_type);
    stbds_hmdel(ctx_estorage.fn_component_to_string, component_type);
    stbds_hmdel(ctx_estorage.index_components, component_type);

    return true;
}

bool itu_sys_ecs_component_is_enabled_ex(ITU_ComponentType component_type)
{
    return stbds_hmget(ctx_estorage.index_components, component_type);
}

const char* itu_sys_ecs_component_debug_name_get_ex(ITU_ComponentType component_type)
{
    ITU_Component* pool = stbds_hmget(ctx_estorage.index_components, component_type);
    if(!pool)
    {
        itu_ecs_set_last_error("component not enabled");
        return NULL;
    }

    return stbds_hmget(ctx_estorage.debug_names_component, component_type);
}

void itu_sys_ecs_component_fn_tostring_set_ex(ITU_ComponentType component_type, ITU_FN_ComponendToString fn_tostring)
{
    ITU_Component* pool = stbds_hmget(ctx_estorage.index_components, component_type);
    if(!pool)
    {
        itu_ecs_set_last_error("component not enabled");
        return;
    }

    stbds_hmput(ctx_estorage.fn_component_to_string, component_type, fn_tostring);
}

void itu_sys_ecs_component_fn_debug_ui_set_ex(ITU_ComponentType component_type, ITU_FN_ComponendDebugRender fn_debug_ui)
{
    ITU_Component* pool = stbds_hmget(ctx_estorage.index_components, component_type);
    if(!pool)
    {
        itu_ecs_set_last_error("component not enabled");
        return;
    }

    stbds_hmput(ctx_estorage.fn_component_debug_ui_render, component_type, fn_debug_ui);
}


bool itu_sys_ecs_system_add_ex(ITU_FN_SystemUpdate fn_update, ITU_SystemQueue queue, Uint64 mask_components, Uint64 mask_tags, const char* debug_name)
{
    if(itu_sys_ecs_is_locked())
    {
        itu_ecs_set_last_error("cannot add systems while updating");

        // TODO add command queues (component stuff might just need to happen outside systems)

        return false;
    }

    if(fn_update == 0)
    {
        itu_ecs_set_last_error("system update function must be non-NULL");
        return false;
    }

    if(ctx_estorage.systems_count[queue] == SYSTEMS_COUNT_MAX)
    {
        itu_ecs_set_last_error("maximum number of systems reached");
        return false;
    }

    // NOTE: getting the first free system id, but not incrementing it
    //       This means we can start setting it up before being sure if the input config is
    //       valid, since an early exit will just modify an invalid entry anyway.
    //       This ALSO mean that we need to zero out all the counters whenever we create a new 
    //       one, as we could have some data from a previous, broken setup!
    // FIXME: this won't work if we destroy systems! System destruction is already a problem
    //        (we can't just swap invalid to the end, system execution order matters!)
    Uint64 system_id = ctx_estorage.systems_count[queue];

    ITU_System* system_runtime = &ctx_estorage.systems[queue][system_id];

    system_runtime->components_count = 0;
    while(mask_components)
    {
        Uint64 high_bit = hibit(mask_components);
        mask_components &= ~high_bit; // sets hi_bit to 0

        int loc = stbds_hmgeti(ctx_estorage.index_components, high_bit);
        if(loc == -1)
        {
            itu_ecs_set_last_error("invalid component %d", high_bit);
            return false;
        }

        if(system_runtime->components_count >= SYSTEM_COMPONENTS_MAX)
        {
            itu_ecs_set_last_error("systems can process a maximum of %s(%d)", ITU_STRINGIFY(SYSTEM_COMPONENTS_MAX), SYSTEM_COMPONENTS_MAX);
            return false;
        }
        system_runtime->components[system_runtime->components_count++] = ctx_estorage.index_components[loc].value;
    }

    // TODO revisit tags
    system_runtime->mask_tags = mask_tags;

    // NOTE seems like everything else is making copies
    system_runtime->fn_update = fn_update;
    ctx_estorage.debug_names_system[queue][system_id] = debug_name;

    // system valid an initialized

    ctx_estorage.systems_count[queue]++;

    return true;
}

bool itu_sys_ecs_system_disable(ITU_FN_SystemUpdate system, ITU_SystemQueue queue)
{
    if(itu_sys_ecs_is_locked())
    {
        itu_ecs_set_last_error("cannot remove systems while updating");

        // TODO add command queues (component stuff might just need to happen outside systems)

        return false;
    }

    // NOTE this method is very inefficient:
    //      - linear search
    //      - copy of small-ish struct (144bytes at the time of writing)
    //      probably fine as system swapping should be relatively infrequent.
    //      before adding anything more complex, it could be worth offering an option to
    //      disable componens in batches, as that's probably the most common case



    // NOTE system execution order is important! when removing, we can jast swap with last
    Uint64 i = 0;
    while(i < ctx_estorage.systems_count[queue] && ctx_estorage.systems[queue][i].fn_update != system)
        ++i;

    // NOTE systems do not make copies of their debug names, so no need to free here
    // TODO check and uniform all debug name handling (ATM some make copies, some hold
    //      references, this is not good)
    while(i < ctx_estorage.systems_count[queue] - 1)
    {
        ctx_estorage.systems[queue][i] = ctx_estorage.systems[queue][i + 1];
        ctx_estorage.debug_names_system[queue][i] = ctx_estorage.debug_names_system[queue][i + 1];
    }
    ctx_estorage.systems_count[queue]--;

    return true;
}

const char* itu_sys_ecs_system_queue_debug_name_get(ITU_SystemQueue queue)
{
    switch(queue)
    {
        case ITU_SYSTEM_QUEUE_UPDATE       : return "update";
        case ITU_SYSTEM_QUEUE_PHYSICS_PRE  : return "physics_pre";
        case ITU_SYSTEM_QUEUE_PHYSICS_POST : return "physics_post";
        case ITU_SYSTEM_QUEUE_MAX          : return "<UNKNOWN QUEUE>";
    }

    return "<UNKNOWN QUEUE>";
}


// =============================================================================================
// Section: internal methods implementation
// =============================================================================================

void itu_ecs_entities_do_destruction()
{
    int count = stbds_arrlen(ctx_estorage.entities_pending_deletion);

    for(int i = 0; i < count; ++i)
    {
        ITU_EntityId id = ctx_estorage.entities_pending_deletion[i];
        if(itu_sys_ecs_entity_is_valid(id))
            itu_ecs_entity_do_destruction(id);
    }

    stbds_arrsetlen(ctx_estorage.entities_pending_deletion, 0);
}

void itu_ecs_entity_do_destruction(ITU_EntityId id)
{
    SDL_assert(itu_sys_ecs_entity_is_valid(id));

    SDL_Log("[DESTROY] %d %d", id.generation, id.index);

    // NOTE this shouldn't be needed, we should never let client-side access an invalid entity
    // clear debug name
    // itu_sys_ecs_entity_debug_name_set(id, "");

    // free all component data
    Uint64 mask_components = ctx_estorage.entities[id.index].component_mask;
    while(mask_components)
    {
        Uint64 high_bit = hibit(mask_components);
        mask_components &= ~high_bit; // sets hi_bit to 0
        ITU_Component* component = stbds_hmget(ctx_estorage.index_components, high_bit);
        SDL_assert(component);
        itu_ecs_component_pool_remove(component, id);
    }

    // clear all tags
    Uint64 mask_tags = ctx_estorage.entities[id.index].tag_mask;
    while(mask_tags)
    {
        ITU_TagType type = hibit(mask_tags);
        mask_tags &= type - 1; // flips most-significant bit to 0
        Uint64 loc = itu_ecs_tag_mask_to_id(type);
        itu_ecs_tag_pool_remove(ctx_estorage.tags[loc], id);
    }

    // NOTE: order of these two operations is important
    //       1. store the id of the entity we are deleting, WITH THE CURRENT GENERATION
    //       2. set generation in entity storage to invalid
    //       this way, when we'll recycle this id in the future, we know what was the last used
    //       generation
    stbds_arrpush(ctx_estorage.entities_free, ctx_estorage.entities[id.index].id);
    ctx_estorage.entities[id.index].id.generation = ITU_ENTITY_ID_INVALID_GENERATION;
}

int itu_ecs_system_get_matching_entities(ITU_System* system, ITU_EntityId* out_entity_group)
{
    SDL_assert(system);
    // SDL_assert(out_entity_group);

    ITU_EntityId* candidate_ids;
    Uint32        candidate_ids_count = UINT32_MAX;
    Uint32        final_ids_count = 0;

    for(Uint32 j = 0; j < system->components_count; ++j)
        if(system->components[j]->count_alive < candidate_ids_count)
        {
            candidate_ids = system->components[j]->entity_ids;
            candidate_ids_count = system->components[j]->count_alive;
        }

    // TODO revisit tags
    Uint64 mask_tags = system->mask_tags;
    while(mask_tags)
    {
        ITU_TagType type = hibit(mask_tags);
        mask_tags &= ~type;
        Uint64 loc = itu_ecs_tag_mask_to_id(type);

        Uint64 count = ctx_estorage.tags[loc]->count_alive;
        if(count < candidate_ids_count)
        {
            candidate_ids = ctx_estorage.tags[loc]->entity_ids;
            candidate_ids_count = count;
        }
    }

    for(Uint32 j = 0; j < system->tags_count; ++j)
    {
        ITU_ComponentTagStorage* tmp = ctx_estorage.tags[system->tags[j]];
        if(!tmp)
            continue;
        if(stbds_hmlen(tmp) < candidate_ids_count)
        {
            candidate_ids = (ITU_EntityId*)tmp;
            candidate_ids_count = stbds_hmlen(tmp);
        }
    }

    // NOTE the only reason this assertion can be true is if we have a system that ONLY uses
    //      components/tags that haven't been activated. It's _almost_ an assertion, not sure
    //      I want to go all the way there.
    if(candidate_ids_count == UINT32_MAX)
    {
        itu_ecs_set_last_error("system is not using any component/tag currently enabled???");
        return 0;
    }

    // filter entities
    for(Uint64 k = 0; k < candidate_ids_count; ++k)
    {
        // FIXME NEXT why certain things are not cleared correctly? (remove tranform for  e00 in dev_06 to see)
        ITU_EntityId entity_curr = candidate_ids[k];
        bool filter_out = false;
        for(Uint32 j = 0; j < system->components_count; ++j)
        {
            ITU_Component* filtered_component = system->components[j];

            if(filtered_component->data_loc[entity_curr.index] == (Uint64)-1)
            {
                filter_out = true;
                break;
            }
        }

        // // TODO revisit tags
        // for(Uint32 j = 0; j < system->tags_count; ++j)
        // {
        //     ITU_TagType filtered_tag = system->tags[j];
                
        //     if(stbds_hmgeti(ctx_estorage.tags[filtered_tag], entity_curr) == -1)
        //     {
        //         filter_out = true;
        //         break;
        //     }
        // }

        if(!filter_out)
        {
            if(out_entity_group)
                out_entity_group[final_ids_count] = entity_curr;
            ++final_ids_count;
        }
    }

    return final_ids_count;
}

void itu_ecs_component_pool_assign(ITU_Component* component_pool, ITU_EntityId entity)
{
    SDL_assert(component_pool);
    SDL_assert(component_pool->count_alive < component_pool->count_max);

    // TODO check that requested entry is actually free

    Uint64 i = component_pool->count_alive++;
    component_pool->data_loc[entity.index] = i;
    component_pool->entity_ids[i] = entity;
    SDL_memset((unsigned char*)component_pool->data + component_pool->element_size * i, 0, component_pool->element_size);
}

void itu_ecs_component_pool_remove(ITU_Component* component_pool, ITU_EntityId entity_id)
{
    SDL_assert(component_pool);

    SDL_Log("\t[DESTROY] pool %s", stbds_hmget(ctx_estorage.debug_names_component, component_pool->type));

    // TODO destruction callbacks?
    if(component_pool->type == Component_PhysicsBody)
        itu_sys_ecs_entity_remove_b2body(entity_id);

    // 1. find location entity to be deleted
    Uint64         swap_loc = component_pool->data_loc[entity_id.index];
    ITU_EntityId   swap_id  = component_pool->entity_ids[swap_loc];
    SDL_assert(itu_sys_ecs_entity_equals(swap_id, entity_id));

    // 2. find location of last entity in array
    Uint64         last_loc = component_pool->count_alive - 1;
    ITU_EntityId   last_id  = component_pool->entity_ids[last_loc];

    // 3. overwrite location of current entity with last entity
    component_pool->entity_ids[swap_loc] = last_id;
    component_pool->data_loc[last_id.index] = swap_loc;

    // 4. overwrite data of current entity with last entity
    Uint64 element_size = component_pool->element_size;
    unsigned char* last_ptr = pointer_index(component_pool->data, last_loc, element_size);
    unsigned char* swap_ptr = pointer_index(component_pool->data, swap_loc, element_size);
    SDL_memcpy(swap_ptr, last_ptr, element_size);

    // 5. clear data location of removed entity
    component_pool->data_loc[entity_id.index] = -1;

    component_pool->count_alive--;
}

void itu_ecs_component_pool_data_get(ITU_Component* component_pool, ITU_EntityId entity, void* out_data_copy)
{
    SDL_assert(out_data_copy);

    void* data = itu_ecs_component_pool_data_get_ptr(component_pool, entity);
    SDL_memcpy(out_data_copy, data, component_pool->element_size);
}

void itu_ecs_component_pool_data_set(ITU_Component* component_pool, ITU_EntityId entity, void* in_data_copy)
{
    SDL_assert(in_data_copy);

    void* data = itu_ecs_component_pool_data_get_ptr(component_pool, entity);
    SDL_memcpy(data, in_data_copy, component_pool->element_size);
}

void *itu_ecs_component_pool_data_get_ptr(ITU_Component* component_pool, ITU_EntityId entity)
{
    SDL_assert(component_pool);

    Uint64 loc = component_pool->data_loc[entity.index];
    return pointer_offset(void, component_pool->data, component_pool->element_size * loc);
}

void itu_ecs_tag_pool_assign(ITU_ComponentTagStorage* tag_pool, ITU_EntityId id)
{
    SDL_assert(tag_pool);
    SDL_assert(tag_pool->count_alive < tag_pool->count_max);

    tag_pool->loc[id.index] = tag_pool->count_alive;
    tag_pool->entity_ids[tag_pool->count_alive] = id;
    tag_pool->count_alive++;
}

void itu_ecs_tag_pool_remove(ITU_ComponentTagStorage* tag_pool, ITU_EntityId id)
{
    SDL_assert(tag_pool);

    // 1. find location entity to be deleted
    Uint64         swap_loc = tag_pool->loc[id.index];
    ITU_EntityId   swap_id  = tag_pool->entity_ids[swap_loc];
    SDL_assert(itu_sys_ecs_entity_equals(swap_id, id));

    // 2. find location of last entity in array
    Uint64         last_loc = tag_pool->count_alive - 1;
    ITU_EntityId   last_id  = tag_pool->entity_ids[last_loc];

    // 3. overwrite location of current entity with last entity
    tag_pool->entity_ids[swap_loc] = last_id;
    tag_pool->loc[last_id.index] = swap_loc;

    // 4. clear data location of removed entity
    tag_pool->loc[id.index] = -1;

    tag_pool->count_alive--;
}


void itu_ecs_set_last_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SDL_vsnprintf(ctx_estorage.str_last_error, STR_LAST_ERROR_SIZE, fmt, args);
    va_end(args);

    // TODO handle priority
    // if(priority >= ctx_estorage.immediate_log_priority)
    {
        SDL_LogMessage(0, SDL_LOG_PRIORITY_ERROR, "%s", ctx_estorage.str_last_error);
    }
    // // FIXME
    // // if(priority ? ctx_estorage.immediate_panic_priority)
    // if(SDL_strcmp(fmt, "No errors yet"))
    // {
    //     SDL_assert(false);
    // }
}
#ifndef ITU_SYS_ECS_INTERNAL_HPP
#define ITU_SYS_ECS_INTERNAL_HPP

#ifndef ITU_UNITY_BUILD
#include <SDL3/SDL.h>
#include <itu_sys_ecs.hpp>
#endif // ITU_UNITY_BUILD

const Uint32 STR_LAST_ERROR_SIZE          = 128;
const Uint32 STR_ENTITY_DEBUG_NAME_SIZE   = 64;
const Uint32 STARTING_FREE_ENTITIES_COUNT = 32;

const Uint64 ITU_ECS_CONTEXT_FLAGS_NONE                = 0b000;
const Uint64 ITU_ECS_CONTEXT_FLAGS_INITIALIZED         = 0b001;
const Uint64 ITU_ECS_CONTEXT_FLAGS_UPDATING            = 0b010;
const Uint64 ITU_ECS_CONTEXT_FLAGS_LOCKED              = 0b100;

struct ITU_Entity
{
    ITU_EntityId      id;
    ITU_ComponentType component_mask;
    ITU_TagType       tag_mask;
};

// FIXME: this name is probably not meaningful anymore! (we need to rework the tags anyway)
struct ITU_ComponentTagStorage
{
    // NOTE oder of these payoff fields is important! We allocate all of the data contiguously,
    //      so we need to set this pointers precisely where they will start.
    //      If the order in which they are defined changes, the tag creation needs to be
    //      updated accordingly
    Uint64*       loc;         // maps EntityId.index to location in the entity set
    ITU_EntityId* entity_ids;  // contiguous list of all entities with this tag

    Uint64        count_max;   // tag capacity
    Uint64        count_alive; // how many entities currently have this tag
};

struct ITU_Component
{
    ITU_ComponentType type;

    // NOTE oder of these payoff fields is important! We allocate all of the data contiguously,
    //      so we need to set this pointers precisely where they will start.
    //      If the order in which they are defined changes, the component creation needs to be
    //      updated accordingly
    Uint64*       data_loc;   // maps EntityId.index to location in data array
    ITU_EntityId* entity_ids; // maps data array location to an EntityId
    void*         data;

    Uint64 element_size;
    Uint32 count_max;
    Uint32 count_alive;
};

struct ITU_System
{
    ITU_Component* components[SYSTEM_COMPONENTS_MAX];
    ITU_TagType    tags[SYSTEM_TAGS_MAX];
    ITU_TagType mask_tags;

    Uint32 components_count;
    Uint32 tags_count;

    ITU_FN_SystemUpdate fn_update;
};

struct ITU_EntityStorageContext
{
    Uint64 flags;

    // NOTE don't get tricked by the fact that this is implemented with an array, this is a map!
    //      From `entity_id` to entity data. Is it worth to do it this way, compared to an
    //      actual hashmap or similar? Can't say without thorougly debugging it, but it's worth
    //      pointing out that it will be common to create/destroy entities in batches, and
    //      unless our hashmap is speciically designed to handle batch updates we might trigger
    //      a lot of rebalancing at the same time, which is not great. (Also, we don't have an
    //      API for batch updates, so the point is a bit moot anyway)
    stbds_arr(ITU_Entity)   entities;
    stbds_arr(ITU_EntityId) entities_free;

    // auxiliary array used while iterating filtered entities for systems
    stbds_arr(ITU_EntityId) entities_system_filtered;
    stbds_arr(ITU_EntityId) entities_pending_deletion;

    stbds_hm(ITU_ComponentType, ITU_Component*) index_components;

    // TODO revisit tags
    ITU_ComponentTagStorage* tags[TAGS_COUNT_MAX];


    ITU_System systems[ITU_SYSTEM_QUEUE_MAX][SYSTEMS_COUNT_MAX];

    Uint32 tags_count;
    Uint32 systems_count[ITU_SYSTEM_QUEUE_MAX];

    // debug properties
    char* debug_names_entity;
    const char* debug_names_system[ITU_SYSTEM_QUEUE_MAX][SYSTEMS_COUNT_MAX];
    const char* debug_names_tag[TAGS_COUNT_MAX];

    // NOTE components contain arbitrary data, so they need arbitrary UI widgets
    // NOTE we're starting to do a bit too many hasmap lookups. This might be the moment
    //      were we group all component data in a single hashmap
    //      (something like struct { ITU_Component, fn_render, fn_to_string, ...} )
    stbds_hm(ITU_ComponentType, const char*)                 debug_names_component;
    stbds_hm(ITU_ComponentType, ITU_FN_ComponendDebugRender) fn_component_debug_ui_render;
    stbds_hm(ITU_ComponentType, ITU_FN_ComponendToString)    fn_component_to_string;

    char* str_last_error;
};

extern ITU_EntityStorageContext ctx_estorage;


// TODO make a public version of this?
int itu_ecs_system_get_matching_entities(ITU_System* system, ITU_EntityId* out_entity_group);

#endif // ITU_SYS_ECS_INTERNAL_HPP
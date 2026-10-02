// itu_sys_ecs_default_systems.hpp

#ifndef ITU_SYS_ECS_DEFAULT_SYSTEMS_HPP
#define ITU_SYS_ECS_DEFAULT_SYSTEMS_HPP

#include <itu_engine.hpp>

ITU_ECS_USE_AS_COMPONENT(Transform2D);
ITU_ECS_USE_AS_COMPONENT(Sprite);

ITU_ECS_COMPONENT(PhysicsWorld)
{
    b2WorldId id;
};

ITU_ECS_USE_AS_COMPONENT(PhysicsCallbacks);

ITU_ECS_COMPONENT(PhysicsBody)
{
    b2BodyId id;
};

void sys_render_sprite(EngineContext* context, ITU_EntityId* entity_ids, Uint64 entity_ids_count);
void sys_render_sprite_debug(EngineContext* context, ITU_EntityId* entity_ids, Uint64 entity_ids_count);
void sys_physics_world_step(EngineContext* context, ITU_EntityId* entity_ids, Uint64 entity_ids_count);
void sys_physics_world_step_callbacks(EngineContext* context, ITU_EntityId* entity_ids, Uint64 entity_ids_count);

// FIXME this should not be callable by the user
PhysicsBody itu_sys_ecs_entity_add_b2body(ITU_EntityId id_entity, b2WorldId id_world, b2BodyDef* def);
// FIXME this should not be callable by the user
bool        itu_sys_ecs_entity_remove_b2body(ITU_EntityId id_entity);

#endif // ITU_SYS_ECS_DEFAULT_SYSTEMS_HPP

#if (defined ITU_SYS_ECS_DEFAULT_SYSTEMS_IMPLEMENTATION) || (defined ITU_UNITY_BUILD)

// filter: Transform2D, Sprite
inline void sys_render_sprite(EngineContext* context, ITU_EntityId* entity_ids, Uint64 entity_ids_count)
{
    for(Uint64 i = 0; i < entity_ids_count; ++i)
    {
        ITU_EntityId id = entity_ids[i];
        Sprite*      sprite   = itu_sys_ecs_entity_component_data_ptr(id, Sprite);
        Transform2D* transform = itu_sys_ecs_entity_component_data_ptr(id, Transform2D);

        itu_lib_sprite_render(context, sprite, transform);
    }
}

// filter: Transform2D, Sprite
inline void sys_render_sprite_debug(EngineContext* context, ITU_EntityId* entity_ids, Uint64 entity_ids_count)
{
    if(!context->debug_ui_show)
        return;

    for(Uint64 i = 0; i < entity_ids_count; ++i)
    {
        ITU_EntityId id = entity_ids[i];
        Sprite*      sprite    = itu_sys_ecs_entity_component_data_ptr(id, Sprite);
        Transform2D* transform = itu_sys_ecs_entity_component_data_ptr(id, Transform2D);

        itu_lib_sprite_render_debug(context, sprite, transform);
    }
}

// filter: PhysicsWorld
inline void sys_physics_world_step(EngineContext* context, ITU_EntityId* entity_ids, Uint64 entity_ids_count)
{
    for(Uint64 i = 0; i < entity_ids_count; ++i)
    {
        ITU_EntityId id = entity_ids[i];
        PhysicsWorld* world = itu_sys_ecs_entity_component_data_ptr(id, PhysicsWorld);

        b2World_Step(
            world->id,
            NS_TO_SECONDS(context->target_framerate_fixed_ns),
            4
        );

        // copy transforms
        b2BodyEvents events_body = b2World_GetBodyEvents(world->id);
        for(int i = 0; i < events_body.moveCount; ++i)
        {
            b2BodyMoveEvent* event     = &events_body.moveEvents[i];
            ITU_EntityId     id_entity = itu_sys_ecs_entity_id_form_generic_pointer(event->userData);

            Transform2D* transform = itu_sys_ecs_entity_component_data_ptr(id_entity, Transform2D);
            if(transform)
            {
                transform->position.x = event->transform.p.x;
                transform->position.y = event->transform.p.y;
                transform->rotation   = b2Rot_GetAngle(event->transform.q);
            }
        }
    }
}

// filter: PhysicsWorld, PhysicsCallbacks
inline void sys_physics_world_step_callbacks(EngineContext* context, ITU_EntityId* entity_ids, Uint64 entity_ids_count)
{
    for(Uint64 i = 0; i < entity_ids_count; ++i)
    {
        ITU_EntityId id = entity_ids[i];
        PhysicsWorld* world        = itu_sys_ecs_entity_component_data_ptr(id, PhysicsWorld);
        PhysicsCallbacks* callbacks = itu_sys_ecs_entity_component_data_ptr(id, PhysicsCallbacks);

        b2BodyEvents events_body = b2World_GetBodyEvents(world->id);
        if(callbacks->fn_body_move)
            for(int i = 0; i < events_body.moveCount; ++i)
            {
                b2BodyMoveEvent* event     = &events_body.moveEvents[i];
                callbacks->fn_body_move(context, event->transform, event->userData, event->fellAsleep);
            }

        b2ContactEvents events_contact = b2World_GetContactEvents(world->id);
        if(callbacks->fn_contact_beg)
            for(int i = 0; i < events_contact.beginCount; ++i)
            {
                b2ContactBeginTouchEvent* event = &events_contact.beginEvents[i];

                b2BodyId id_body_a = b2Shape_GetBody(event->shapeIdA);
                b2BodyId id_body_b = b2Shape_GetBody(event->shapeIdB);
                void* user_data_a = b2Body_GetUserData(id_body_a);
                void* user_data_b = b2Body_GetUserData(id_body_b);

                callbacks->fn_contact_beg(
                    context,
                    user_data_a,
                    event->shapeIdA,
                    user_data_b,
                    event->shapeIdB,
                    &event->manifold
                );

                if(callbacks->asymmetric_contact_callbacks)
                    callbacks->fn_contact_beg(
                        context,
                        user_data_b,
                        event->shapeIdB,
                        user_data_a,
                        event->shapeIdA,
                        &event->manifold
                    );
            }

        if(callbacks && callbacks->fn_contact_end)
            for(int i = 0; i < events_contact.endCount; ++i)
            {
                b2ContactEndTouchEvent* event = &events_contact.endEvents[i];

                // NOTE as per docs, we receive end touch events after a body/shape is destroyed, so we have to check
                //      https://box2d.org/documentation/group__events.html#structb2_contact_end_touch_event
                bool is_valid_a = b2Shape_IsValid(event->shapeIdA);
                bool is_valid_b = b2Shape_IsValid(event->shapeIdB);

                // if both involved bodies are already destroyed there is no point doing anything
                // BUT! if ons is still valid, we still need to rely the info!
                if(!is_valid_a && !is_valid_b)
                    continue;

                void* user_data_a = NULL;
                void* user_data_b = NULL;

                if(is_valid_a)
                {
                    b2BodyId id_body_a = b2Shape_GetBody(event->shapeIdA);
                    user_data_a = b2Body_GetUserData(id_body_a);
                }
                if(is_valid_b)
                {
                    b2BodyId id_body_b = b2Shape_GetBody(event->shapeIdB);
                    user_data_b = b2Body_GetUserData(id_body_b);
                }

                callbacks->fn_contact_end(
                    context,
                    user_data_a,
                    event->shapeIdA,
                    user_data_b,
                    event->shapeIdB
                );
                if(callbacks->asymmetric_contact_callbacks)
                    callbacks->fn_contact_end(
                        context,
                        user_data_b,
                        event->shapeIdB,
                        user_data_a,
                        event->shapeIdA
                    );
            }

        if(callbacks && callbacks->fn_contact_hit)
            for(int i = 0; i < events_contact.hitCount; ++i)
            {
                b2ContactHitEvent* event = &events_contact.hitEvents[i];

                // NOTE as per docs, we receive end touch events after a body/shape is destroyed, so we have to check
                //      https://box2d.org/documentation/group__events.html#structb2_contact_end_touch_event
                bool is_valid_a = b2Shape_IsValid(event->shapeIdA);
                bool is_valid_b = b2Shape_IsValid(event->shapeIdB);

                // if both involved bodies are already destroyed there is no point doing anything
                // BUT! if ons is still valid, we still need to rely the info!
                if(!is_valid_a && !is_valid_b)
                    continue;

                void* user_data_a = NULL;
                void* user_data_b = NULL;

                if(is_valid_a)
                {
                    b2BodyId id_body_a = b2Shape_GetBody(event->shapeIdA);
                    user_data_a = b2Body_GetUserData(id_body_a);
                }
                if(is_valid_b)
                {
                    b2BodyId id_body_b = b2Shape_GetBody(event->shapeIdB);
                    user_data_b = b2Body_GetUserData(id_body_b);
                }

                callbacks->fn_contact_hit(
                    context,
                    user_data_a,
                    event->shapeIdA,
                    user_data_b,
                    event->shapeIdB,
                    { event->point.x, event->point.y },
                    { event->normal.x, event->normal.y },
                    event->approachSpeed
                );
                if(callbacks->asymmetric_contact_callbacks)
                    callbacks->fn_contact_hit(
                        context,
                        user_data_b,
                        event->shapeIdB,
                        user_data_a,
                        event->shapeIdA,
                        { event->point.x, event->point.y },
                        { event->normal.x, event->normal.y },
                        event->approachSpeed
                    );
            }

        b2SensorEvents  events_sensor  = b2World_GetSensorEvents(world->id);
        if(callbacks && callbacks->fn_sensor_beg)
            for(int i = 0; i < events_sensor.beginCount; ++i)
            {
                b2SensorBeginTouchEvent* event = &events_sensor.beginEvents[i];
                b2BodyId id_body_sensor  = b2Shape_GetBody(event->sensorShapeId);
                b2BodyId id_body_visitor = b2Shape_GetBody(event->visitorShapeId);
                void* user_data_sensor   = b2Body_GetUserData(id_body_sensor);
                void* user_data_visitor  = b2Body_GetUserData(id_body_visitor);

                callbacks->fn_sensor_beg(
                    context,
                    user_data_sensor,
                    event->sensorShapeId,
                    user_data_visitor,
                    event->visitorShapeId
                );
            }

        if(callbacks && callbacks->fn_sensor_end)
            for(int i = 0; i < events_sensor.endCount; ++i)
            {
                b2SensorEndTouchEvent* event = &events_sensor.endEvents[i];
                b2BodyId id_body_sensor  = b2Shape_GetBody(event->sensorShapeId);
                b2BodyId id_body_visitor = b2Shape_GetBody(event->visitorShapeId);
                void* user_data_sensor   = b2Body_GetUserData(id_body_sensor);
                void* user_data_visitor  = b2Body_GetUserData(id_body_visitor);

                callbacks->fn_sensor_end(
                    context,
                    user_data_sensor,
                    event->sensorShapeId,
                    user_data_visitor,
                    event->visitorShapeId
                );
            }
    }
}


inline PhysicsBody itu_sys_ecs_entity_add_b2body(ITU_EntityId id_entity, b2WorldId id_world, b2BodyDef* def)
{
    def->userData = itu_sys_ecs_entity_id_to_generic_pointer(id_entity); // FIXME very dangerous, will break if we make ITU_EntityId bigger!
    PhysicsBody body = { b2CreateBody(id_world, def) };
    itu_sys_ecs_entity_component_set(id_entity, PhysicsBody, &body);

    return body;

}

inline bool itu_sys_ecs_entity_remove_b2body(ITU_EntityId id_entity)
{
    PhysicsBody body;
    if(!itu_sys_ecs_entity_component_get(id_entity, PhysicsBody, &body))
    {
        SDL_Log("Entity %s does not have a physics body!", itu_sys_ecs_entity_debug_name_get(id_entity));
        return false;
    }

    b2DestroyBody(body.id);
    return true;
}

#endif // (defined ITU_SYS_ECS_DEFAULT_SYSTEMS_IMPLEMENTATION) || (defined ITU_UNITY_BUILD)
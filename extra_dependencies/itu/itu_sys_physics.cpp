#include <itu_engine.hpp>

void TMP_log_event(const char* prefix, b2ShapeId a, b2ShapeId b);

void itu_sys_physics_default_debuggin_setup(EngineContext* context, b2DebugDraw* debug_draw)
{
    SDL_assert(debug_draw);
    SDL_memset(debug_draw, 0, sizeof(b2DebugDraw));

    debug_draw->context = context;
    debug_draw->drawShapes = true;
    debug_draw->DrawSolidPolygonFcn = fn_box2d_wrapper_draw_polygon;
    debug_draw->DrawSolidCircleFcn  = fn_box2d_wrapper_draw_circle;
    debug_draw->DrawSolidCapsuleFcn = fn_box2d_wrapper_draw_capsule;
}

// // FIXME
// extern const ITU_ComponentType Component_Transform2D;

void itu_sys_physics_loop(EngineContext* context, void* game_state, b2WorldId b2_world_id, const PhysicsCallbacks* callbacks)
{
    context->physics_steps_per_frame_count = 0;
    while(
        context->accumulator_physics >= (SDL_Time)context->target_framerate_fixed_ns &&
        context->physics_steps_per_frame_count < context->config.physics_steps_per_frame_max
    )
    {
        b2World_Step(b2_world_id, NS_TO_SECONDS(context->target_framerate_fixed_ns), 4);
        ++context->physics_steps_per_frame_count;
        context->accumulator_physics -= context->target_framerate_fixed_ns;

        b2BodyEvents events_body = b2World_GetBodyEvents(b2_world_id);
        for(int i = 0; i < events_body.moveCount; ++i)
        {
            b2BodyMoveEvent* event     = &events_body.moveEvents[i];
            if(callbacks && callbacks->fn_body_move)
                callbacks->fn_body_move(context, event->transform, event->userData, event->fellAsleep);
        }

        b2SensorEvents  events_sensor  = b2World_GetSensorEvents(b2_world_id);
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

        b2ContactEvents events_contact = b2World_GetContactEvents(b2_world_id);
        if(callbacks && callbacks->fn_contact_beg)
            for(int i = 0; i < events_contact.beginCount; ++i)
                {
                    b2ContactBeginTouchEvent* event = &events_contact.beginEvents[i];
                    TMP_log_event("[BEG]", event->shapeIdA, event->shapeIdB);

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
                TMP_log_event("[END]", event->shapeIdA, event->shapeIdB);

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
                TMP_log_event("[HIT]", event->shapeIdA, event->shapeIdB);

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

        if(callbacks && callbacks->fn_update)
            callbacks->fn_update(context, game_state);
    }
}

b2Polygon itu_sys_physycs_polygon_from_sprite(EngineContext* context, Sprite* sprite, Transform2D* transform)
{
    vec2f size   = itu_lib_sprite_get_world_size(context, sprite, transform);
    vec2f offset = mul_element_wise(size, sprite->pivot);

    //vec2f offset_a = offset;
    //vec2f offset_b = { 1.0f - offset.x, 1.0f - offset.y };
    vec2f pos = size - offset;
    vec2f neg = -offset;
    b2Vec2 points[4];
    points[0] = { neg.x, neg.y };
    points[1] = { pos.x, neg.y };
    points[2] = { pos.x, pos.y };
    points[3] = { neg.x, pos.y };
    b2Hull hull = b2ComputeHull(points, 4);
    b2Polygon poly = b2MakePolygon(&hull, 0);
    return poly;
}












// internal methods
void TMP_log_event(const char* prefix, b2ShapeId a, b2ShapeId b)
{

    const char* name_a = NULL;
    const char* name_b = NULL;

    if(b2Shape_IsValid(a))
    {
        b2BodyId id_body_a = b2Shape_GetBody(a);
            name_a = b2Body_GetName(id_body_a);}
    if(b2Shape_IsValid(b))
        {
    b2BodyId id_body_b = b2Shape_GetBody(b);
            name_b = b2Body_GetName(id_body_b);}
    printf(
        "\t%s [%c]%8s [%c]%8s\n",
        prefix,
        b2Shape_IsValid(a) ? '_' : '!',
        name_a,
        b2Shape_IsValid(b) ? '_' : '!',
        name_b
    );
}



// b2BodyId itu_sys_physics_add_body(void* entity, b2BodyDef* body_def)
// {
//     b2BodyId ret = b2CreateBody(sys_physics_data.world_id, body_def);
//     stbds_hmput(sys_physics_data.map_b2body_entity, ret, entity);

//     return ret;
// }

// void* itu_sys_physics_get_entity(b2BodyId body_id)
// {
//     return stbds_hmget(sys_physics_data.map_b2body_entity, body_id);
// }

// b2SensorEvents itu_sys_physics_get_sensor_events()
// {
//     b2SensorEvents ret = b2World_GetSensorEvents(sys_physics_data.world_id);
//     return ret;
// }

// b2ContactEvents itu_sys_physics_get_contact_events()
// {
//     b2ContactEvents ret = b2World_GetContactEvents(sys_physics_data.world_id);
//     return ret;
// }

// void itu_sys_physics_debug_draw()
// {
//     b2World_Draw(sys_physics_data.world_id, &sys_physics_data.debug_draw);
// }


// wrapper around box2D
// we are almost sandboxing box2d, but we are still using its def-structures for convenience

#ifndef ITU_SYS_PHYSICS_HPP
#define ITU_SYS_PHYSICS_HPP

#ifndef ITU_UNITY_BUILD
#include <box2d/box2d.h>
#include <itu_lib_context.hpp>
#include <utils/itu_utils_box2d.hpp>
#include <itu_lib_render2d.hpp>
#include <itu_lib_transform2d.hpp>
#endif

typedef void (*FN_CALLBACK_SENSOR)        (EngineContext* context, void* sensor, b2ShapeId shape_sensor, void* visitor, b2ShapeId shape_visitor);
typedef void (*FN_CALLBACK_CONTACT_BEG)   (EngineContext* context, void* a, b2ShapeId shape_a, void* b, b2ShapeId shape_b, b2Manifold* manifold);
typedef void (*FN_CALLBACK_CONTACT_END)   (EngineContext* context, void* a, b2ShapeId shape_a, void* b, b2ShapeId shape_b);
typedef void (*FN_CALLBACK_CONTACT_HIT)   (EngineContext* context, void* a, b2ShapeId shape_a, void* b, b2ShapeId shape_b, vec2f point, vec2f normal, float approach_speed);
typedef void (*FN_CALLBACK_BODY_MOVE)     (EngineContext* context, b2Transform transform, void* user_data, bool fell_asleep);
typedef void (*FN_CALLBACK_PHYSICS_UPDATE)(EngineContext* context, void* state);

struct PhysicsCallbacks
{
    FN_CALLBACK_SENSOR         fn_sensor_beg  = NULL;
    FN_CALLBACK_SENSOR         fn_sensor_end  = NULL;
    FN_CALLBACK_CONTACT_BEG    fn_contact_beg = NULL;
    FN_CALLBACK_CONTACT_END    fn_contact_end = NULL;
    FN_CALLBACK_CONTACT_HIT    fn_contact_hit = NULL;
    FN_CALLBACK_BODY_MOVE      fn_body_move   = NULL;
    FN_CALLBACK_PHYSICS_UPDATE fn_update      = NULL;

    // when true, fires the contact callbacks two times, with the entities swapped
    // 
    // applies only to `fn_contact_beg`, `fn_contact_end` and `fn_contact_hit`
    bool asymmetric_contact_callbacks = false;
};

void itu_sys_physics_default_debuggin_setup(EngineContext* context, b2DebugDraw* debug_draw);
void itu_sys_physics_loop(EngineContext* context, void* game_state, b2WorldId world, const PhysicsCallbacks* callbacks);
b2Polygon itu_sys_physycs_polygon_from_sprite(EngineContext* context, Sprite* sprite, Transform2D* transform);


b2BodyId       itu_sys_physics_add_body(void* entity, b2BodyDef* body_def);
b2ShapeId      itu_sys_physics_add_shape(b2BodyId body, b2BodyDef* body_def);
void*          itu_sys_physics_get_entity(b2BodyId body_id);
b2SensorEvents itu_sys_physics_get_sensor_events();
void           itu_sys_physics_debug_draw();

#endif // ITU_SYS_PHYSICS_HPP

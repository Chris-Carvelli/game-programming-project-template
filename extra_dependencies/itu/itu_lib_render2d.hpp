#ifndef ITU_LIB_RENDER_2D_HPP
#define ITU_LIB_RENDER_2D_HPP

#ifndef ITU_UNITY_BUILD
#include <SDL3/SDL.h>
#include <itu_lib_render_screen.hpp>
#include <itu_lib_transform2d.hpp>
#include <itu_lib_context.hpp>
#endif

struct Sprite
{
	SDL_Texture* texture;
	SDL_FRect    rect;
	vec2f        pivot;
	color        tint;
	bool         flip_horizontal;
};

void      itu_lib_sprite_init(Sprite* sprite, SDL_Texture* texture, SDL_FRect rect);
SDL_FRect itu_lib_sprite_get_source_rect(int x, int y, int tile_w, int tile_h);
SDL_FRect itu_lib_sprite_get_screen_rect(EngineContext* context, Sprite* sprite, Transform2D* transform);
vec2f     itu_lib_sprite_get_world_size(EngineContext* context, Sprite* sprite, Transform2D* transform);
void      itu_lib_sprite_render(EngineContext* context, Sprite* sprite, Transform2D* transform);
void      itu_lib_sprite_render_debug(EngineContext* context, Sprite* sprite, Transform2D* transform);

void      itu_lib_render_draw_world_point(EngineContext* context, vec2f pos, float half_size, color color);
void      itu_lib_render_draw_world_line(EngineContext* context, vec2f p0, vec2f p1, color color);
void      itu_lib_render_draw_world_rect(EngineContext* context, vec2f min, vec2f max, color color);
void      itu_lib_render_draw_world_rect_fill(EngineContext* context, vec2f min, vec2f max, color color);
void      itu_lib_render_draw_world_circle(EngineContext* context, vec2f center, float radius, int vertex_count, color);
void      itu_lib_render_draw_world_polygon(EngineContext* context, vec2f position, const vec2f* vertices, int vertexCount, color color);

void      itu_lib_render_draw_world_grid(EngineContext* context);

#endif // ITU_LIB_RENDER_2D_HPP

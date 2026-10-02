#include <itu_engine.hpp>

#ifndef ITU_UNITY_BUILD
#include <SDL3/SDL.h>
#endif

// inits sprite with reasonable defaults
void itu_lib_sprite_init(Sprite* sprite, SDL_Texture* texture, SDL_FRect rect)
{
	sprite->texture = texture;
	sprite->rect = rect;
	sprite->pivot = vec2f{ 0.5f, 0.5f };
	sprite->tint = COLOR_WHITE;
}

SDL_FRect itu_lib_sprite_get_source_rect(int x, int y, int tile_w, int tile_h)
{
	SDL_FRect ret;
	ret.x = x * tile_w;
	ret.y = y * tile_h;
	ret.w = tile_w;
	ret.h = tile_h;
	return ret;
}

SDL_FRect itu_lib_sprite_get_screen_rect(EngineContext* context, Sprite* sprite, Transform2D* transform)
{
	vec2f sprite_size_world;
	sprite_size_world.x = sprite->rect.w / context->config.texture_pixels_per_unit;
	sprite_size_world.y = sprite->rect.h / context->config.texture_pixels_per_unit;

	SDL_FRect rect_dst;
	rect_dst.w = transform->scale.x * sprite_size_world.x;
	rect_dst.h = transform->scale.y * sprite_size_world.y;
	rect_dst.x = transform->position.x - sprite->pivot.x * rect_dst.w;
	rect_dst.y = transform->position.y - sprite->pivot.y * rect_dst.h;
	rect_dst = itu_lib_context_rect_global_to_screen(context, rect_dst);

	return rect_dst;
}

vec2f itu_lib_sprite_get_world_size(EngineContext* context, Sprite* sprite, Transform2D* transform)
{
	vec2f sprite_size_world;
	sprite_size_world.x = sprite->rect.w / context->config.texture_pixels_per_unit;
	sprite_size_world.y = sprite->rect.h / context->config.texture_pixels_per_unit;
	return sprite_size_world;
}

void itu_lib_sprite_render(EngineContext* context, Sprite* sprite, Transform2D* transform)
{
	SDL_FRect rect_src = sprite->rect;
	SDL_FRect rect_dst = itu_lib_sprite_get_screen_rect(context, sprite, transform);
	SDL_FPoint pivot_dst;
	pivot_dst.x = (1.0f - sprite->pivot.x) * rect_dst.w;
	pivot_dst.y = (1.0f - sprite->pivot.y) * rect_dst.h;

	SDL_SetTextureColorModFloat(sprite->texture, sprite->tint.r, sprite->tint.g, sprite->tint.b);
	SDL_SetTextureAlphaModFloat(sprite->texture, sprite->tint.a);
	SDL_RenderTextureRotated(
		context->renderer,
		sprite->texture,
		&rect_src,
		&rect_dst,
		(-transform->rotation) * RAD_2_DEG,
		&pivot_dst,
		sprite->flip_horizontal ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE
	);
}

void itu_lib_sprite_render_debug(EngineContext* context, Sprite* sprite, Transform2D* transform)
{
	vec2f pos = itu_lib_context_point_global_to_screen(context, transform->position);
	SDL_FRect rect = itu_lib_sprite_get_screen_rect(context,  sprite, transform);

	itu_lib_render_screen_rect_rotated(context->renderer, vec2f{ rect.x, rect.y }, vec2f{ rect.w, rect.h }, pos, transform->rotation, COLOR_WHITE);
	itu_lib_render_screen_point(context->renderer, pos, 5, COLOR_YELLOW);
}


void itu_lib_render_draw_world_point(EngineContext* context, vec2f pos, float half_size, color color)
{
	itu_lib_render_screen_point(context->renderer, itu_lib_context_point_global_to_screen(context, pos), half_size, color);
}

void itu_lib_render_draw_world_line(EngineContext* context, vec2f p0, vec2f p1, color color)
{
	itu_lib_render_screen_line(context->renderer, itu_lib_context_point_global_to_screen(context, p0), itu_lib_context_point_global_to_screen(context, p1), color);
}

void itu_lib_render_draw_world_rect(EngineContext* context, vec2f min, vec2f max, color color);
void itu_lib_render_draw_world_rect_fill(EngineContext* context, vec2f min, vec2f max, color color);
void itu_lib_render_draw_world_circle(EngineContext* context, vec2f center, float radius, int vertex_count, color c)
{
	itu_lib_render_screen_circle(context->renderer, itu_lib_context_point_global_to_screen(context, center), itu_lib_context_size_global_to_screen(context, radius), vertex_count, c);
}

void itu_lib_render_draw_world_polygon(EngineContext* context, vec2f position, const vec2f* vertices, int vertexCount, color color)
{
	itu_lib_render_screen_polygon(context->renderer, itu_lib_context_point_global_to_screen(context, position), vertices, vertexCount, color);
}

void itu_lib_render_draw_world_grid(EngineContext* context)
{
	// const float spacing_min = 32;
	// const float spacing_max = 128;

	Camera* camera = context->camera_active;

	float spacing = spacing = camera->pixels_per_unit * camera->zoom;

	// float scaling_factor = 1;
	// while(spacing > spacing_max)
	// {
	// 	spacing /= 2;
	// 	scaling_factor /= 2;
	// }
	// while(spacing < spacing_min)
	// {
	// 	spacing *= 2;
	// 	scaling_factor *= 2;
	// }


	vec2f screen_size = vec2f { context->sdl_window_w, context->sdl_window_h };
	vec2f camera_world_min = camera->world_position - screen_size / (camera->pixels_per_unit * camera->zoom * 2);
	vec2f camera_world_max = camera->world_position + screen_size / (camera->pixels_per_unit * camera->zoom * 2);
	camera_world_min.x = ((int)camera_world_min.x - 1);
	camera_world_min.y = ((int)camera_world_min.y - 1);
	camera_world_max.x = ((int)camera_world_max.x + 1);
	camera_world_max.y = ((int)camera_world_max.y + 1);

	vec2f camera_window_min = itu_lib_context_point_global_to_screen(context, camera_world_min);
	vec2f camera_window_max = itu_lib_context_point_global_to_screen(context, camera_world_max);

	float tmp = camera_window_min.y;
	camera_window_min.y = camera_window_max.y;
	camera_window_max.y = tmp;


	vec2f offset;
	offset.x = 0;
	offset.y = 0;

	SDL_SetRenderDrawColorFloat(context->renderer, 0.7f, 0.7f, 0.7f, 0.5f);
	float min_x = camera_window_min.x;
	float min_y = camera_window_min.y;

	for(float i = min_x + offset.x; i <= camera_window_max.x; i += spacing)
		SDL_RenderLine(context->renderer, i, camera_window_min.y, i, camera_window_max.y);

	for(float i = min_y + offset.y; i <= camera_window_max.y; i += spacing)
		SDL_RenderLine(context->renderer, camera_window_min.x, i, camera_window_max.x, i);
}

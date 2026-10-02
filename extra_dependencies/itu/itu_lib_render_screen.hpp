// itu_lib_renderer.hpp
// simple library to render debug shapes

#ifndef ITU_LIB_RENDER_SCREEN_HPP
#define ITU_LIB_RENDER_SCREEN_HPP

#ifndef ITU_UNITY_BUILD
#include <SDL3/SDL.h>
#include <itu_common.hpp>
#endif // ITU_UNITY_BUILD

#define MAX_CIRCLE_VERTICES  16
#define MAX_POLYGON_VERTICES 32

// =====================================================================================================================
// publi API
// =====================================================================================================================


void itu_lib_render_screen_point(SDL_Renderer* renderer, vec2f pos, float half_size, color color);
void itu_lib_render_screen_line(SDL_Renderer* renderer, vec2f p0, vec2f p1, color color);
void itu_lib_render_screen_rect(SDL_Renderer* renderer, vec2f min, vec2f max, color color);
void itu_lib_render_screen_rect_rotated(SDL_Renderer* renderer, vec2f min, vec2f extents, vec2f pivot, float angle, color color);
void itu_lib_render_screen_rect_fill(SDL_Renderer* renderer, vec2f min, vec2f max, color color);
void itu_lib_render_screen_circle(SDL_Renderer* renderer, vec2f center, float radius, int vertex_count, color color);
void itu_lib_render_screen_polygon(SDL_Renderer* renderer, vec2f position, const vec2f* vertices, int vertexCount, color color);

#endif // ITU_LIB_RENDER_SCREEN_HPP

#if (defined ITU_LIB_RENDER_SCREEN_IMPLEMENTATION) || (defined ITU_UNITY_BUILD)

// =====================================================================================================================
// implementations
// =====================================================================================================================


void itu_lib_render_screen_point(SDL_Renderer* renderer, vec2f pos, float half_size, color color)
{
	//itu_lib_render_screen_rect(renderer, pos - vec2f { size / 2, size / 2}, vec2f { size, size }, color);
	SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b,color.a);
	SDL_RenderLine(renderer, pos.x - half_size, pos.y, pos.x + half_size, pos.y);
	SDL_RenderLine(renderer, pos.x, pos.y - half_size, pos.x, pos.y + half_size);
}

void itu_lib_render_screen_line(SDL_Renderer* renderer, vec2f p0, vec2f p1, color color)
{
	SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b,color.a);
	SDL_RenderLine(renderer, p0.x, p0.y, p1.x, p1.y);
}

void itu_lib_render_screen_rect(SDL_Renderer* renderer, vec2f min, vec2f extents, color color)
{
	SDL_FRect rect;
	rect.x = min.x;
	rect.y = min.y;
	rect.w = extents.x;
	rect.h = extents.y;

	SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderRect(renderer, &rect);
}

void itu_lib_render_screen_rect_rotated(SDL_Renderer* renderer, vec2f min, vec2f extents, vec2f pivot, float angle, color color)
{
	// TODO build a proper rotate around function
	vec2f max = min + extents;

	const float s = SDL_sinf(-angle);
	const float c = SDL_cosf(-angle);
	float s_minx = s * (min.x - pivot.x);
	float s_miny = s * (min.y - pivot.y);
	float s_maxx = s * (max.x - pivot.x);
	float s_maxy = s * (max.y - pivot.y);
	float c_minx = c * (min.x - pivot.x);
	float c_miny = c * (min.y - pivot.y);
	float c_maxx = c * (max.x - pivot.x);
	float c_maxy = c * (max.y - pivot.y);


	SDL_FPoint points[5];
	points[0].x = (c_minx - s_miny) + pivot.x;
	points[0].y = (s_minx + c_miny) + pivot.y;
	points[1].x = (c_maxx - s_miny) + pivot.x;
	points[1].y = (s_maxx + c_miny) + pivot.y;
	points[2].x = (c_maxx - s_maxy) + pivot.x;
	points[2].y = (s_maxx + c_maxy) + pivot.y;
	points[3].x = (c_minx - s_maxy) + pivot.x;
	points[3].y = (s_minx + c_maxy) + pivot.y;
	points[4].x = points[0].x;
	points[4].y = points[0].y;

	SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderLines(renderer, points, 5);
}

void itu_lib_render_screen_rect_fill(SDL_Renderer* renderer, vec2f min, vec2f extents, color color)
{
	SDL_FRect rect;
	rect.x = min.x;
	rect.y = min.y;
	rect.w = extents.x;
	rect.h = extents.y;

	SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect);
}

// NOTE: vertex count must be smaller than `MAX_CIRCLE_VERTICES` (defaults to 16, but you can change it if you need to)
void itu_lib_render_screen_circle(SDL_Renderer* renderer, vec2f center, float radius, int vertex_count, color color)
{
	SDL_assert(vertex_count <= MAX_CIRCLE_VERTICES);

	SDL_FPoint points[MAX_CIRCLE_VERTICES + 1];
	
	float angle_increment = TAU / vertex_count;

	// very slow, a lot of trig
	// we could also do a single quadrant and mirror the rest
	for(int i = 0; i < vertex_count; ++i)
	{
		float angle = angle_increment * i;
		float px = radius * SDL_cos(angle);
		float py = radius * SDL_sin(angle);
		points[i].x = center.x + px;
		points[i].y = center.y + py;
	}
	points[vertex_count] = points[0];
	
	SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, 0xFF);
	SDL_RenderLines(renderer, points, vertex_count + 1);
}

void itu_lib_render_screen_polygon(SDL_Renderer* renderer, vec2f position, const vec2f* vertices, int vertexCount, color color)
{
	SDL_FColor color_fill = { color.r, color.g, color.b, color.a };

	SDL_FPoint vs_outline[MAX_POLYGON_VERTICES];
	SDL_Vertex vs[MAX_POLYGON_VERTICES];
	SDL_zeroa(vs);
	
	for (int i = 0; i < vertexCount; ++i)
	{
		vec2f pos = position + vertices[i];

		vs[i].color = color_fill;
		vs[i].position.x = pos.x;
		vs[i].position.y = pos.y;
		vs[i].tex_coord.x = 0;
		vs[i].tex_coord.y = 0;
		vs_outline[i].x = vs[i].position.x;
		vs_outline[i].y = vs[i].position.y;
	}
	vs_outline[vertexCount].x = vs_outline[0].x;
	vs_outline[vertexCount].y = vs_outline[0].y;

	int indices_count = (vertexCount - 2)*3;
	int indices[MAX_POLYGON_VERTICES];
	int c = 0;
	for (int i = 2; i < vertexCount; ++i)
	{
		indices[c++] = 0;
		indices[c++] = i - 1;
		indices[c++] = i;
	}

	SDL_RenderGeometry(renderer, NULL, vs, vertexCount, indices, indices_count);
	
	SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, 1.0f);
	SDL_RenderLines(renderer, vs_outline, vertexCount + 1);
}

# endif // (defined ITU_LIB_RENDER_SCREEN_IMPLEMENTATION) || (defined ITU_UNITY_BUILD)

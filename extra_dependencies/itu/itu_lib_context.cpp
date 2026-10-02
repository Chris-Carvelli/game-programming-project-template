#include <itu_engine.hpp>


// forward declarations
bool itu_lib_imgui_process_sdl_event(SDL_Event* event);
void itu_lib_imgui_set_scale(float zoom);

void itu_lib_context_init(EngineConfig* config, EngineContext* context)
{
	SDL_assert(context); // context must be not-null
	context->config = *config;
	context->working_dir = SDL_GetCurrentDirectory();
	context->target_framerate_fluid_ns = SECONDS(1) / config->step_per_second_fluid;
	context->target_framerate_fixed_ns = SECONDS(1) / config->step_per_second_fixed;
	context->sdl_window_w = context->config.window_w;
	context->sdl_window_h = context->config.window_h;

	// FIXME: duplicated proprerty! Either
	//        1. use config also at runtime, except for computed properties like target_framerate_fluid_ns
	//        2. copy/compute all properties, and carry around config only for reset purposes
	context->zoom = config->zoom;


	context->window   = SDL_CreateWindow(config->application_name, config->window_w, config->window_h, 0);

	// TODO: uniform way that renderer 2D and 3D are initialized
	//       currently, renderer3D sets a minimal but complete environment with almost no config from user,
	//       whil renderer2D only creates the renderer and then askes the user to create camera, load
	//       textures and so on.
	//       This makes sense for the course, since for 2D we expect the students to do/learn most of the stuff
	//       themself also at a low-ish level, while 3D is just a tease and only from a client-side perspective
	if(config->type_renderer == TYPE_RENDERER_2D)
		context->renderer = SDL_CreateRenderer(context->window, NULL);

	itu_lib_context_set_zoom(context, config->zoom);

	// FIXES: camera improvements
	// 1. store a reference to the SDL context (ugly as hell, but this way we avoid changing all the functions that take a `Camera*` into)
	// 2. size is now expressed as normalized window size (name should be updated, but again we want to update old exercises
	context->camera_default.normalized_screen_size.x = 1.0f;
	context->camera_default.normalized_screen_size.y = 1.0f;
	context->camera_default.zoom = 1;
	context->camera_default.pixels_per_unit = config->camera_pixel_per_unit;

	itu_lib_context_set_active_camera(context, &context->camera_default);

	SDL_SetRenderDrawBlendMode(context->renderer, SDL_BLENDMODE_BLEND);
}

void itu_lib_context_set_zoom(EngineContext* context, float zoom)
{

	if(context->imgui_io)
		itu_lib_imgui_set_scale(zoom);

	context->zoom = zoom;

	if(context->config.type_renderer == TYPE_RENDERER_3D)
	{
		SDL_Log("setting the zoom at the engine level works only for 2D! With 3D rendering it might have unforseen consequence!\n");
		return;
	}

	// NOTE: this is the bit that will confuse the 3D renderer. Our simple 3D "renderer" doesn't handle having a
	//       render surface of a different size than the native window.
	context->sdl_window_w = context->config.window_w / zoom;
	context->sdl_window_h = context->config.window_h / zoom;

	// NOTE: this affects both `SDL_Renderer` and `SDL_GPUDevice`, but it's is mostly used for resolution independence
	//       (which 3D should handle with projection matrix). We use it for scaling debug text and imgui when rendering on a
	//       projector. Changing zoom on 3D will scale down the rendering surface (sort of like changing the viewport), so
	//       we end up with empty patches of screen. It should either never happen, or it should also update the projection matrix to
	//       fill the window correctly. Also, imgui needs to be notified when zoom changes, but right now it happens on init only
	SDL_SetRenderScale(context->renderer, zoom, zoom);
}

void itu_lib_context_set_active_camera(EngineContext* context, Camera* camera)
{
	context->camera_active = camera;

	SDL_Rect rect;
	rect.w = context->sdl_window_w * camera->normalized_screen_size.x;
	rect.h = context->sdl_window_h * camera->normalized_screen_size.y;
	rect.x = context->sdl_window_w * camera->normalized_screen_offset.x;
	rect.y = context->sdl_window_h * camera->normalized_screen_offset.y;

	SDL_SetRenderViewport(context->renderer, &rect);
}

SDL_FRect itu_lib_camera_get_viewport_rect(EngineContext* context, Camera* camera)
{
	SDL_FRect rect;
	rect.w = context->sdl_window_w * camera->normalized_screen_size.x;
	rect.h = 0.99f * context->sdl_window_h * camera->normalized_screen_size.y;
	rect.x = 0.99f * context->sdl_window_w * camera->normalized_screen_offset.x;
	rect.y = 0.99f * context->sdl_window_h * camera->normalized_screen_offset.y;

	return rect;
}

// converts the given rect to the viewport of the active camera
SDL_FRect itu_lib_context_rect_global_to_screen(EngineContext* context, SDL_FRect rect)
{
	SDL_assert(context);
	Camera* camera = context->camera_active;

	SDL_assert(camera);

	vec2f camera_size;
	camera_size.x = (context->sdl_window_w / camera->pixels_per_unit) * camera->normalized_screen_size.x;
	camera_size.y = (context->sdl_window_h / camera->pixels_per_unit) * camera->normalized_screen_size.y;

	vec2f camera_offset;
	camera_offset.x = (context->sdl_window_w / camera->pixels_per_unit)* camera->normalized_screen_offset.x;
	camera_offset.y = (context->sdl_window_h / camera->pixels_per_unit)* camera->normalized_screen_offset.y;

	vec2f pos  = vec2f{ rect.x, rect.y };
	vec2f size = vec2f{ rect.w, rect.h };

	pos = pos - camera->world_position;
	pos = pos * camera->zoom;
	// pos = pos + camera->normalized_screen_size / 2;
	// pos.y = camera->normalized_screen_size.y - pos.y - size.y * camera->zoom;
	pos = pos + camera_size / 2;
	pos.y = camera_size.y - pos.y - size.y * camera->zoom;
	
	SDL_FRect ret;
	ret.w = camera->pixels_per_unit * size.x * camera->zoom;
	ret.h = camera->pixels_per_unit * size.y * camera->zoom;
	// ret.x = camera->pixels_per_unit * pos.x;
	// ret.y = camera->pixels_per_unit * pos.y;
	ret.x = camera->pixels_per_unit * pos.x + camera_offset.x;
	ret.y = camera->pixels_per_unit * pos.y + camera_offset.y;

	return ret;
}

float itu_lib_context_size_global_to_screen(EngineContext* context, float size)
{
	SDL_assert(context);
	Camera* camera = context->camera_active;

	SDL_assert(camera);

	return size * camera->pixels_per_unit * camera->zoom;
}

// converts the given point to the viewport of the given camera
vec2f itu_lib_context_point_global_to_screen(EngineContext* context, vec2f p)
{
	if(!context)
		SDL_Log("context: %p\n", context);
	SDL_assert(context);
	Camera* camera = context->camera_active;

	SDL_assert(camera);

	vec2f camera_size;
	camera_size.x = (context->sdl_window_w / camera->pixels_per_unit) * camera->normalized_screen_size.x;
	camera_size.y = (context->sdl_window_h / camera->pixels_per_unit) * camera->normalized_screen_size.y;
	
	vec2f camera_offset;
	camera_offset.x = (context->sdl_window_w / camera->pixels_per_unit)* camera->normalized_screen_offset.x;
	camera_offset.y = (context->sdl_window_h / camera->pixels_per_unit)* camera->normalized_screen_offset.y;

	vec2f ret = p;
	ret = ret - camera->world_position;
	ret = ret * camera->zoom;
	ret = ret + camera_size / 2;
	ret.y = camera_size.y - ret.y;
	ret = ret * camera->pixels_per_unit + camera_offset;

	return ret;
}

// converts the given point from the viewport of the given camera to world space
vec2f itu_lib_context_point_screen_to_global(EngineContext* context, vec2f p)
{
	SDL_assert(context);
	Camera* camera = context->camera_active;

	SDL_assert(camera);

	vec2f camera_size;
	camera_size.x = (context->sdl_window_w / camera->pixels_per_unit) * camera->normalized_screen_size.x;
	camera_size.y = (context->sdl_window_h / camera->pixels_per_unit) * camera->normalized_screen_size.y;

	vec2f camera_offset;
	camera_offset.x = (context->sdl_window_w / camera->pixels_per_unit)* camera->normalized_screen_offset.x;
	camera_offset.y = (context->sdl_window_h / camera->pixels_per_unit)* camera->normalized_screen_offset.y;

	vec2f ret = p;
	ret = ret / camera->pixels_per_unit;
	ret.y = camera_size.y - ret.y;
	ret = ret - camera_offset;
	ret = ret - camera_size / 2;
	ret = ret / camera->zoom;
	ret = ret + camera->world_position;

	return ret;
}

vec2f itu_lib_context_point_screen_to_window(EngineContext* context, vec2f p)
{
	SDL_assert(context);
	Camera* camera = context->camera_active;

	SDL_assert(camera);

	vec2f native_window_size = { (float)context->config.window_w, (float)context->config.window_h };
	vec2f ret = p + mul_element_wise(camera->normalized_screen_offset, native_window_size);
	return ret;
}

vec2f itu_lib_context_point_window_to_screen(EngineContext* context, vec2f p)
{
	SDL_assert(context);
	Camera* camera = context->camera_active;

	SDL_assert(camera);

	vec2f native_window_size = { (float)context->config.window_w, (float)context->config.window_h };
	vec2f ret = p - mul_element_wise(camera->normalized_screen_offset, native_window_size);
	return ret;
}

void itu_lib_input_set_mapping_keyboard(EngineContext* context, SDL_Keycode key, BtnType input)
{
	stbds_hmput(context->mappings_keyboard, key, input);
}

// NOTE: mouse button mapping is a bit more lose than keyboard mapping. The most common one is:
//       1: left button
//       2: middle button
//       3: right button
//       if that doesn' work for you, try adding a log in `itu_lib_input_process_events` to find the correct ones
void itu_lib_input_set_mapping_mouse(EngineContext* context, Uint8 key, BtnType input)
{
	stbds_hmput(context->mappings_mouse, key, input);
}

// prepare input data structures to receive information about this frame input data from SDL event queue
//
// NOTE: call this every frame before `sdl_process_event!`
void itu_lib_input_clear(EngineContext* context)
{
	context->mouse_scroll = 0;
	//context->mouse_delta.x = 0;
	//context->mouse_delta.y = 0;

	for(int i = 0; i < BTN_TYPE_MAX; ++i)
		context->btn_isjustpressed[i] = false;
}

// Auxiliary function to process low-frequency inputs.
//
// May drop inputs that are firing higher than the current framerate
void itu_lib_input_key_process(EngineContext* context, BtnType button_id, SDL_Event* event)
{
	context->btn_isdown[button_id] = event->key.down;
	context->btn_isjustpressed[button_id] = event->key.down && !event->key.repeat;
}

// Auxiliary function to process low-frequency inputs.
//
// May drop inputs that are firing higher than the current framerate
void itu_lib_input_mouse_button_process(EngineContext* context, BtnType button_id, SDL_Event* event)
{
	context->btn_isjustpressed[button_id] = event->button.down && !context->btn_isjustpressed[button_id];
	context->btn_isdown[button_id] = event->button.down;
}

// processes all inputs in the queue
// in a real implementation we would impose a fixed cap on the number of inputs we process every frame, how much time we 
// spend in the loop, or both (if we don't, we might get stuck in an infinite loop, or spend a noticeable amount of time
// processing inputs).
//
//In practice, this is rarely a problem, so we won;t bother for the exercises
bool itu_lib_input_process_events(EngineContext* context)
{
	// input
	bool ret = false;
	SDL_Event event;
	itu_lib_input_clear(context);

	while(SDL_PollEvent(&event))
	{
		if(context->imgui_io && itu_lib_imgui_process_sdl_event(&event))
			continue;
		switch(event.type)
		{
			case SDL_EVENT_QUIT:
				ret = true;
				break;
			// listen for mouse motion and store the absolute position in screen space
			case SDL_EVENT_MOUSE_MOTION:
			{
				context->mouse_pos.x = event.motion.x;
				context->mouse_pos.y = event.motion.y;
				context->mouse_delta.x = event.motion.xrel;
				context->mouse_delta.y = event.motion.yrel;
				break;
			}
			// listen for mouse wheel and store the relative position in screen space
			case SDL_EVENT_MOUSE_WHEEL:
			{
				context->mouse_scroll = event.wheel.y;
				break;
			}
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
			{
				int i = stbds_hmgeti(context->mappings_mouse, event.button.button);
				if(i != -1)
					itu_lib_input_mouse_button_process(context, context->mappings_mouse[i].value, &event);
				break;
			}
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
			{
				int i = stbds_hmgeti(context->mappings_keyboard, event.key.key);
				if(i != -1)
					itu_lib_input_key_process(context, context->mappings_keyboard[i].value, &event);

				break;
			}
		}
	}

	return ret;
}

SDL_Texture* itu_resources_texture_create(EngineContext* context, const char* path, SDL_ScaleMode mode)
{
	// texture could accept which pixel format it has as parameter, but this for now seems good enough
	const SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_RGBA32;

	// number of parameters is determined by the pixel format. If that is allowed to change in the future,
	// we will need to acquire the correct one through some kind of mapping
	const int num_components_requested = 4;

	int w=0, h=0, n=0;
	unsigned char* pixels = itu_resources_texture_load_raw(path, num_components_requested, &w, &h, &n);
	
	SDL_Surface* surface = SDL_CreateSurfaceFrom(w, h, pixel_format, pixels, w * num_components_requested);

	SDL_Texture* ret = SDL_CreateTextureFromSurface(context->renderer, surface);
	SDL_SetTextureScaleMode(ret, mode);

	SDL_DestroySurface(surface);
	stbi_image_free(pixels);

	return ret;
}

Uint8* itu_resources_texture_load_raw(const char* path, int num_channels_requested, int* w, int* h, int* num_channels_read)
{
	unsigned char* pixels = stbi_load(path, w, h, num_channels_read, num_channels_requested);

	SDL_assert(pixels);

	return pixels;
};

// busy waits to introduce artificial delay
void itu_lib_context_artificial_delay(float delay_ms, float delay_spread_ms)
{
	SDL_Time walltime_start;
	SDL_Time walltime_busywait;
	SDL_Time target_wait = (delay_ms + (SDL_randf() - 0.5f) * delay_spread_ms) * 1000000;

	SDL_GetCurrentTime(&walltime_start);
	walltime_busywait = walltime_start;
	while(walltime_busywait - walltime_start < target_wait)
		SDL_GetCurrentTime(&walltime_busywait);
}

void itu_lib_context_frame_timing_setup(EngineContext* context)
{
	SDL_GetCurrentTime(&context->walltime_frame_beg);
	context->walltime_frame_end = context->walltime_frame_beg;
}

void itu_lib_context_frame_timing_update(EngineContext* context)
{
	SDL_GetCurrentTime(&context->walltime_work_end);
	context->elapsed_work = context->walltime_work_end - context->walltime_frame_beg;

	if(context->elapsed_work < context->target_framerate_fluid_ns)
		SDL_DelayPrecise(context->target_framerate_fluid_ns - context->elapsed_work);

	SDL_GetCurrentTime(&context->walltime_frame_end);
	context->elapsed_frame = context->walltime_frame_end - context->walltime_frame_beg;

	context->delta = (float)context->elapsed_frame / (float)SECONDS(1);
	context->uptime += context->delta;
	context->accumulator_physics += context->elapsed_frame;
	context->walltime_frame_beg = context->walltime_frame_end;
}


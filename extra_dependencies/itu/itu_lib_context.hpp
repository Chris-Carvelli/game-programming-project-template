// handling of the basic SDL resources, plus some other odds and ends which are not fleshed out enough to get their own
// files
//
// - camera
// - resource loading
// - input
//

// NOTE: we technically don't need include guards since we are mostly doing unity builds, but better safe than sorry
//       review this if we start using precompiler headers
#ifndef ITU_LIB_CONTEXT_HPP
#define ITU_LIB_CONTEXT_HPP

#ifndef ITU_UNITY_BUILD
#include <SDL3/SDL.h>
#include <stb_ds.h>
#include <stb_image.h>
#include <itu_common.hpp>
#include <imgui/imgui.h>
#endif

// forward declarations

// context for 3D rendering functionality. Teaser for lecture 08
struct ITU_Renderer3D; 

enum TypeRenderer
{
	TYPE_RENDERER_2D,
	TYPE_RENDERER_3D
};

enum BtnType
{
	BTN_TYPE_UP,
	BTN_TYPE_DOWN,
	BTN_TYPE_LEFT,
	BTN_TYPE_RIGHT,
	BTN_TYPE_ACTION_0,
	BTN_TYPE_ACTION_1,
	BTN_TYPE_SPACE,
	
	BTN_TYPE_UI_SELECT,
	BTN_TYPE_UI_EXTRA,  // right click

	BTN_TYPE_DEBUG_F1,
	BTN_TYPE_DEBUG_F2,
	BTN_TYPE_DEBUG_F3,
	BTN_TYPE_DEBUG_F4,
	BTN_TYPE_DEBUG_RESET,

	BTN_TYPE_MAX
};


struct Camera
{
	vec2f world_position; // world position
	vec2f normalized_screen_size;     // NORMALIZED size   (inside the screen rect)
	vec2f normalized_screen_offset;   // NORMALIZED offset (inside the screen rect)
	float zoom;
	float pixels_per_unit;
};

struct EngineConfig
{
	const char* application_name = "ITU Engine";
	TypeRenderer type_renderer = TYPE_RENDERER_2D;
	int window_w = 800;                       // native window width
	int window_h = 600;                       // native window height
	float zoom = 1;                           // render zoom for things that do no use cameras (SDL_DebugText, imgui windows)
	int texture_pixels_per_unit = 16;         // base texture "zoom": how many pixels of a texture will be mapped to a single world unit
	int camera_pixel_per_unit   = 32;         // base camera "zoom": how many screen pixels a single unit will cover
	int step_per_second_fluid   = 60;         // how many fps we are **aiming** to achieve for our fluid loop.
	                                          // Sleep if we're above this (to save power), but if we're too slow everything will still work fine
	int step_per_second_fixed   = 60;         // how many fps we are **aiming** to achieve with out fixed loop.
	                                          // Game will NEED to keep this budget in mind. If we are above budget for a few frames it should still be ok,
	                                          // at least for physics, but we risk spiraling out of control
	int physics_steps_per_frame_max = 4;   // maximum amount of physics step allowed per fluid step
	int physics_contacts_per_entity_max = 16; // maximum amount of physics step allowed per fluid step
};

struct EngineContext
{
	EngineConfig config;

	const char* working_dir;

	SDL_Window*     window;
	SDL_Renderer*   renderer;
	ITU_Renderer3D* ctx_rendering;
	ImGuiIO*        imgui_io;

	float zoom;				// render zoom for things that do no use cameras (SDL_DebugText, imgui windows)
	float sdl_window_w;		// rendering surface width  (after render zoom has been applied)
	float sdl_window_h;		// rendering surface height (after render zoom has been applied)

	Uint64 target_framerate_fixed_ns;
	Uint64 target_framerate_fluid_ns;

	float delta;    // in seconds
	float uptime;   // in seconds

	SDL_Time walltime_frame_beg;
	SDL_Time walltime_frame_end;
	SDL_Time walltime_work_end;
	SDL_Time elapsed_work;
	SDL_Time elapsed_frame;
	SDL_Time accumulator_physics;
	int physics_steps_per_frame_count;

	Camera* camera_active;  // currently active camera
	Camera  camera_default; // default camera

	union
	{
		bool btn_isdown[BTN_TYPE_MAX];
		struct
		{
			bool btn_isdown_up;
			bool btn_isdown_down;
			bool btn_isdown_left;
			bool btn_isdown_right;
			bool btn_isdown_action0;
			bool btn_isdown_action1;
			bool btn_isdown_space;
			bool btn_isdown_ui_select;
			bool btn_isdown_ui_extra;
			bool btn_isdown_ui_f1;
			bool btn_isdown_ui_f2;
			bool btn_isdown_ui_f3;
		};
	};

	union
	{
		bool btn_isjustpressed[BTN_TYPE_MAX];
		struct
		{
			bool btn_isjustpressed_up;
			bool btn_isjustpressed_down;
			bool btn_isjustpressed_left;
			bool btn_isjustpressed_right;
			bool btn_isjustpressed_action0;
			bool btn_isjustpressed_action1;
			bool btn_isjustpressed_space;
			bool btn_isjustpressed_ui_select;
			bool btn_isjustpressed_ui_extra;
			bool btn_isjustpressed_ui_f1;
			bool btn_isjustpressed_ui_f2;
			bool btn_isjustpressed_ui_f3;
		};
	};

	vec2f mouse_pos;
	vec2f mouse_delta;
	float mouse_scroll;

	stbds_hm(SDL_Keycode, BtnType) mappings_keyboard;
	stbds_hm(Uint8, BtnType)       mappings_mouse;

	bool debug_ui_show = true;
	bool should_reset  = false;
};


//==============================================================================================================
// public API interface
//==============================================================================================================

// engine stuff
void      itu_lib_context_init(EngineConfig* config, EngineContext* context);
void      itu_lib_context_set_zoom(EngineContext* context, float zoom);
void      itu_lib_context_set_active_camera(EngineContext* context, Camera* camera);
void      itu_lib_context_frame_timing_setup(EngineContext* context);
void      itu_lib_context_frame_timing_update(EngineContext* context);
void      itu_lib_context_artificial_delay(float delay_ms, float delay_spread_ms);

SDL_FRect itu_lib_context_rect_global_to_screen(EngineContext* context, SDL_FRect rect);
float     itu_lib_context_size_global_to_screen(EngineContext* context, float size);
vec2f     itu_lib_context_point_global_to_screen(EngineContext* context, vec2f p);
vec2f     itu_lib_context_point_screen_to_global(EngineContext* context, vec2f p);
vec2f     itu_lib_context_point_screen_to_window(EngineContext* context, vec2f p);
vec2f     itu_lib_context_point_window_to_screen(EngineContext* context, vec2f p);


SDL_FRect itu_lib_camera_get_viewport_rect(EngineContext* context, Camera* camera);

void itu_lib_input_set_mapping_keyboard(EngineContext* context, SDL_Keycode key, BtnType input);
void itu_lib_input_set_mapping_mouse(EngineContext* context, Uint8 key, BtnType input);
void itu_lib_input_clear(EngineContext* context);
void itu_lib_input_key_process(EngineContext* context, BtnType button_id, SDL_Event* event);
void itu_lib_input_mouse_button_process(EngineContext* context, BtnType button_id, SDL_Event* event);
bool itu_lib_input_process_events(EngineContext* context);

// texture RESOURCE stuff
SDL_Texture* itu_resources_texture_create(EngineContext* context, const char* path, SDL_ScaleMode mode);
Uint8*       itu_resources_texture_load_raw(const char* path, int num_channels_requested, int* w, int* h, int* num_channels_read);

#endif // ITU_LIB_CONTEXT_HPP

#ifndef ITU_LIB_IMGUI_HPP
#define ITU_LIB_IMGUI_HPP

#ifndef ITU_UNITY_BUILD
#include <SDL3/SDL.h>
#include <itu_lib_context.hpp>
// #include <itu_sys_render3d.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl3.h>
#include <imgui/imgui_impl_sdlrenderer3.h>
#include <imgui/imgui_impl_sdlgpu3.h>
#include <imgui/imgui_impl_sdlgpu3_shaders.h>
#endif //ITU_UNITY_BUILD

void itu_lib_imgui_setup(EngineContext* context, bool intercept_keyboard);
void itu_lib_imgui_set_scale(float zoom);

// default imgui event handler. Call this before doing your own processing of the SDL event
// NOTE: a return value of `true` is a suggestion to skip processing of the event for the application
bool itu_lib_imgui_process_sdl_event(SDL_Event* event);
void itu_lib_imgui_frame_begin(EngineContext* context);
void itu_lib_imgui_frame_end(EngineContext* context);

#endif // ITU_LIB_IMGUI_HPP

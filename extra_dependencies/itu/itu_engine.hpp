// unity build header
// include this only once, and no other header

#ifndef ITU_ENGINE_HPP
#define ITU_ENGINE_HPP

#define ITU_UNITY_BUILD

// SDL: https://www.libsdl.org/
//      https://wiki.libsdl.org/SDL3/FrontPage#satellite-libraries
// "cross-platform development library designed to provide low level access to audio, keyboard, mouse, joystick, and graphics hardware"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h> // E05
#include <SDL3_ttf/SDL_ttf.h>     // E08


// stb libraries: https://github.com/nothings/stb
// "single-file public domain (or MIT licensed) libraries for C/C++"
#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_ds.h>    //E01
#include <stb_image.h> //E01

// imgui: https://github.com/ocornut/imgui
// "bloat-free graphical user interface library for C++"
// NOTE: intended and designe for **debug** UI and tools (although it's possible to hack for for in-game UI)
#include <imgui/imgui.h>                      // E03
#include <imgui/imgui_impl_sdl3.h>            // E03
#include <imgui/imgui_impl_sdlrenderer3.h>    // E03
#include <imgui/imgui_impl_sdlgpu3.h>         // E03
#include <imgui/imgui_impl_sdlgpu3_shaders.h> // E03

// box2D: https://github.com/erincatto/box2d
// "2D rigid body simulation library for games"
#include <box2d/box2d.h>                      // E04

// glm: https://github.com/g-truc/glm
// "header only C++ mathematics library for graphics software based on the OpenGL Shading Language (GLSL) specifications"
// (ie: similar syntax to the one used in certain shaders, implementation makes it more straightforward to move data
// between CPU and GPU)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>                        // E08
#include <glm/gtc/matrix_transform.hpp>       // E08
#include <glm/gtx/euler_angles.hpp>           // E08
#include <glm/gtx/quaternion.hpp>             // E08
#include <glm/gtx/transform.hpp>              // E08
#include <glm/gtx/matrix_decompose.hpp>       // E08


#ifdef ITU_SYS_RENDER_3D_IMPLEMENTATION
// assimp: https://assimp.org/
// "library that loads various 3D file formats into a shared, in-memory format. It supports more than 40 file formats for import and a growing selection of file formats for export"
#include <assimp/scene.h>                     // E08
#include <assimp/cimport.h>                   // E08
#include <assimp/postprocess.h>               // E08
#endif // ITU_SYS_RENDER_3D_IMPLEMENTATION

// ITU ENGINE

// low level libraries (no context or memory allocation involved)
#include <itu_common.hpp>                     // E02 // cleaned
#include <itu_lib_render_screen.hpp>          // E02 // cleaned
#include <itu_lib_overlaps.hpp>               // E02 // cleaned
#include <itu_lib_transform2d.hpp>            // E03
#include <itu_lib_fileutils.hpp>              // E08
#include <itu_lib_math3d.hpp>                 // E08
#include <itu_lib_bitmanipulation.hpp>        // E05

// mid level libraries, rely on context
#include <itu_lib_context.hpp>         // E03 // cleaned
#include <utils/itu_utils_box2d.hpp>   // E04
#include <itu_lib_render2d.hpp>        // E03 // clenaed
// #include <itu_sys_render3d.hpp>
#include <itu_lib_imgui.hpp>           // E03 // clenaed (mostly)
#include <itu_sys_physics.hpp>         // E05 

// high level libraries (handle resources, entities, and scenes)
// #include <itu_sys_storage_resource.hpp>
// #include <itu_sys_ecs.hpp>
// #include <itu_sys_ecs_internal.hpp>
// #include <debug_ui/itu_sys_ecs_debug_ui.hpp>


// NOTE: some static code analysis will throw an error here mentioning "file cannot be included
//       recursively", or something similar. This is not a real error that will happen when we
//       compile (it might have implications on compile times, but we're gonna ignore that here)
//       If it happens to you and that annoys you, I've wrapped these .cpp inclusions to make it
//       easy to silence this. Just define THE `IN_IDE` preprocessor symbol IN THE TOOL ONLY!
#ifndef IN_IDE
	#include <itu_lib_fileutils.cpp>
	#include <itu_lib_math3d.cpp>

	#include <itu_lib_context.cpp>         // E03 // cleaned
	#include <utils/itu_utils_box2d.cpp>   // E04

	#include <itu_lib_render2d.cpp>        // E03 // clenaed

	#ifdef ITU_SYS_RENDER_3D_IMPLEMENTATION
		#include <itu_sys_render3d.cpp>
	#endif

	#include <itu_lib_imgui.cpp>           // E03 // clenaed (mostly)
	#include <itu_sys_physics.cpp>         // E05 

	// #include <itu_sys_storage_resource.cpp>
	// #include <itu_sys_ecs.cpp>
	// #include <debug_ui/itu_sys_ecs_debug_ui.cpp>
#endif
#endif // ITU_ENGINE_HPP
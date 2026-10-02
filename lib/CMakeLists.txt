set(SDLMIXER_VENDORED ON)
    set(SDLMIXER_VORBIS_VORBISFILE OFF)
    set(SDLMIXER_FLAC_LIBFLAC OFF)
    set(SDLMIXER_OPUS OFF)
    set(SDLMIXER_GME OFF)
    set(SDLMIXER_MOD OFF)
    set(SDLMIXER_MP3_MPG123 OFF)
    set(SDLMIXER_MIDI OFF)

set(SDLTTF_VENDORED ON)
    set(SDLTTF_HARFBUZZ OFF)
    set(SDLTTF_PLUTOSVG OFF)

# 3rd party libraries
add_subdirectory(SDL)
add_subdirectory(SDL_mixer)
add_subdirectory(SDL_ttf)
add_subdirectory(imgui)
add_subdirectory(box2d)
add_subdirectory(glm)


set(STATIC_LIBS  ${STATIC_LIBS};SDL3::SDL3;SDL3_mixer::SDL3_mixer;SDL3_ttf::SDL3_ttf;box2d::box2d;imgui CACHE INTERNAL "STATIC_LIBS")
set(INCLUDE_DIRS ${INCLUDE_DIRS};${CMAKE_CURRENT_SOURCE_DIR}/stb;${CMAKE_CURRENT_SOURCE_DIR}/stb;${CMAKE_CURRENT_SOURCE_DIR}/imgui;${CMAKE_CURRENT_SOURCE_DIR}/glm;${CMAKE_CURRENT_SOURCE_DIR}/rapidjson CACHE INTERNAL "INCLUDE_DIRS")

message("424243: include dirs:")
foreach(dir ${INCLUDE_DIRS})
    message("\t" ${dir})
endforeach()

message("424243: static libs:")
foreach(lib ${STATIC_LIBS})
    message("\t" ${lib})
endforeach()
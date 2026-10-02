cmake_minimum_required(VERSION 4.0)

project(game-programming-project-template)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

set(BUILD_SHARED_LIBS OFF)

set(STATIC_LIBS  "" CACHE INTERNAL "STATIC_LIBS")
set(INCLUDE_DIRS "" CACHE INTERNAL "INCLUDE_DIRS")

add_subdirectory(lib)

set(INCLUDE_DIRS ${INCLUDE_DIRS};${CMAKE_SOURCE_DIR}/extra_dependencies/itu)

add_subdirectory(src)

# ugly hack to avoid setting the working directory for VSCode (which works for debuggin only)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/data SYMBOLIC)
if(EXISTS ${CMAKE_BINARY_DIR}/Debug)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/Debug/data SYMBOLIC)
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/Release)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/Release/data SYMBOLIC)
endif()


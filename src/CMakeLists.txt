set(targetname "MY_GAME_PLS_CHANGE_MY_NAME")
file(GLOB file_src_list "*.c" "*.cpp")

add_executable(${targetname} ${file_src_list})

target_include_directories(${targetname} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${targetname} PUBLIC ${INCLUDE_DIRS})
target_link_libraries(${targetname} ${STATIC_LIBS})

message("424242: include dirs:")
foreach(dir ${INCLUDE_DIRS})
    message("\t" ${dir})
endforeach()

message("424242: static libs:")
foreach(lib ${STATIC_LIBS})
    message("\t" ${lib})
endforeach()
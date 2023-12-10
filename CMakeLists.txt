cmake_minimum_required(VERSION 3.27)
project(gputracer
    VERSION 1.0
    DESCRIPTION "GPU raytracer"
    LANGUAGES CXX C)
include_directories(../glad/include ../glfw-3.3.8/include ../glm)


add_executable(gputracer gputracer/main.cpp gputracer/glad.c gputracer/imgui.cpp gputracer/imgui_impl_glfw.cpp gputracer/imgui_impl_opengl3.cpp gputracer/imgui_tables.cpp gputracer/imgui_widgets.cpp gputracer/imgui_draw.cpp gputracer/TextEditor.cpp )

add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES IMPORTED_LOCATION ../lib/glfw3.lib)

target_link_libraries(gputracer glfw)
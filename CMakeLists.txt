cmake_minimum_required(VERSION 4.0)
project(CardGameEngine C)

set(CMAKE_C_STANDARD 23)

find_package(glew)
find_package(stb)
find_package(nuklear)
find_package(glfw3)
find_package(linmath.h)

add_executable(CardGameEngine src/main.c
    src/nuklear_glfw_gl4.h
    src/nuklear_include.h
    src/engine.h
    src/engine.c
    src/shl.c
    src/shl.c
        src/editor_ui.h
        src/editor_ui.c
        src/texturepacker.c
        src/texturepacker.h
)

target_link_libraries(CardGameEngine glfw)
target_link_libraries(CardGameEngine nuklear::nuklear)
target_link_libraries(CardGameEngine GLEW::GLEW)
target_link_libraries(CardGameEngine stb::stb)
target_link_libraries(CardGameEngine linmath.h::linmath.h)


file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
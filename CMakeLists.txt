cmake_minimum_required(VERSION 3.12)
project(NutmegEngine C CXX)
project(NutmegEngine C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NUTMEG_BUILD_EDITOR "Build the Nutmeg ImGui editor" OFF)

add_library(nutmeg STATIC
    src/engine.c
    src/builtins.c
)

target_include_directories(nutmeg
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(nutmeg PRIVATE NUTMEG_BUILD)

target_compile_options(nutmeg PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

target_compile_features(nutmeg PUBLIC c_std_99)

if(NOT MSVC)
    target_link_libraries(nutmeg PUBLIC m)
endif()

add_executable(orbit_demo examples/orbit_demo.c)
target_link_libraries(orbit_demo PRIVATE nutmeg)

if(NUTMEG_BUILD_EDITOR)
    include(FetchContent)

    if(NOT DEFINED NUTMEG_IMGUI_DIR)
        FetchContent_Declare(imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.89.9
        )
        FetchContent_MakeAvailable(imgui)
        set(NUTMEG_IMGUI_DIR ${imgui_SOURCE_DIR})
    endif()

    set(IMGUI_DIR ${NUTMEG_IMGUI_DIR})

    add_library(nutmeg_imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp
    )
    target_include_directories(nutmeg_imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)

    find_package(glfw3 QUIET)
    if(NOT glfw3_FOUND)
        find_package(glfw3 REQUIRED)
    endif()

    set(NUTMEG_GLFW_TARGET glfw)
    if(TARGET glfw::glfw)
        set(NUTMEG_GLFW_TARGET glfw::glfw)
    elseif(NOT TARGET glfw)
        message(FATAL_ERROR "Unable to locate a GLFW target for the editor build")
    endif()

    find_package(OpenGL REQUIRED)

    add_executable(nutmeg_editor
        src/editor/editor_app.cpp
        src/editor/editor_ui.cpp
        src/editor/editor_panels.cpp
    )

    target_include_directories(nutmeg_editor
        PRIVATE
            src/editor
            ${IMGUI_DIR}
            ${IMGUI_DIR}/backends
    )

    target_link_libraries(nutmeg_editor
        PRIVATE
            nutmeg
            nutmeg_imgui
            ${NUTMEG_GLFW_TARGET}
            OpenGL::GL
    )

    target_compile_features(nutmeg_editor PRIVATE cxx_std_17)
endif()
add_executable(orbit_demo examples/orbit_demo.c)
target_link_libraries(orbit_demo PRIVATE nutmeg)

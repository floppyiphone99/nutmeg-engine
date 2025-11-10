cmake_minimum_required(VERSION 3.12)
project(NutmegEngine C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

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

add_executable(orbit_demo examples/orbit_demo.c)
target_link_libraries(orbit_demo PRIVATE nutmeg)

cmake_minimum_required(VERSION 2.8)
project(stbaithreshold)
add_compile_options(-O3)
include_directories(src)
add_executable(stbaithreshold src/aithreshold.c src/dependencies.c)
target_link_libraries(stbaithreshold m)

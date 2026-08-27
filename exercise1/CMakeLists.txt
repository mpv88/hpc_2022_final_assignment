cmake_minimum_required(VERSION 3.20)

project(GoL C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(gol
        src/main.c
)

target_compile_options(gol PRIVATE
        -Wall
        -Wextra
        -O3
        -march=native
)
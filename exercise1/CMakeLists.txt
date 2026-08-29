cmake_minimum_required(VERSION 3.20)

project(GoL C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)

add_executable(gol
        src/main.c
)

target_compile_options(gol PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(gol PRIVATE
            -O3
            -march=native
    )
endif()

target_link_libraries(gol PRIVATE OpenMP::OpenMP_C)

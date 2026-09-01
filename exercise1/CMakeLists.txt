cmake_minimum_required(VERSION 3.20)

project(GoL C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)

add_executable(gol
        src/main.c
        src/pgm.c
        include/pgm.h
        src/grid.c
        include/grid.h
)

add_executable(test_pgm
        tests/test_pgm.c
        src/pgm.c
        include/pgm.h
)

target_include_directories(gol PRIVATE include)
target_include_directories(test_pgm PRIVATE include)

target_compile_options(gol PRIVATE
        -Wall -Wextra -Wpedantic
)

target_compile_options(test_pgm PRIVATE
        -Wall -Wextra -Wpedantic
)

target_compile_definitions(test_pgm PRIVATE
        _POSIX_C_SOURCE=200809L
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(gol PRIVATE
            -O3 -march=native
    )
endif()

target_link_libraries(gol PRIVATE OpenMP::OpenMP_C MPI::MPI_C)
target_link_libraries(test_pgm PRIVATE MPI::MPI_C)
add_executable(test_grid tests/test_grid.c)

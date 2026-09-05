cmake_minimum_required(VERSION 3.20)

project(GoL C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)

add_executable(gol_serial
        src/main_serial.c
        src/args.c
        src/pgm.c
        src/grid.c
        src/evolution_common.c
        include/args.h
        include/pgm.h
        include/grid.h
        include/evolution_common.h
        src/evolution_ordered.c
        include/evolution_ordered.h
)

add_executable(gol_parallel
        src/main_parallel.c
        src/args.c
        src/pgm.c
        src/grid.c
        src/evolution_common.c
        include/args.h
        include/pgm.h
        include/grid.h
        include/evolution_common.h
        src/evolution_ordered.c
        include/evolution_ordered.h
)

add_executable(test_pgm
        tests/test_pgm.c
        src/pgm.c
        include/pgm.h
)

add_executable(test_grid
        tests/test_grid.c
        src/grid.c
        include/grid.h
)

add_executable(test_evolution_common
        tests/test_evolution_common.c
        src/evolution_common.c
        include/evolution_common.h
)

target_include_directories(gol_serial PRIVATE include)
target_include_directories(gol_parallel PRIVATE include)
target_include_directories(test_pgm PRIVATE include)
target_include_directories(test_grid PRIVATE include)
target_include_directories(test_evolution_common PRIVATE include)

target_compile_options(gol_serial PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(gol_parallel PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(test_pgm PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(test_grid PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(test_evolution_common PRIVATE -Wall -Wextra -Wpedantic)

target_compile_definitions(gol_serial PRIVATE _POSIX_C_SOURCE=200809L)
target_compile_definitions(test_pgm PRIVATE _POSIX_C_SOURCE=200809L)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(gol_serial PRIVATE -O3 -march=native)
    target_compile_options(gol_parallel PRIVATE -O3 -march=native)
endif()

target_link_libraries(gol_serial PRIVATE
        OpenMP::OpenMP_C
        MPI::MPI_C
)

target_link_libraries(gol_parallel PRIVATE
        OpenMP::OpenMP_C
        MPI::MPI_C
)

target_link_libraries(test_pgm PRIVATE
        MPI::MPI_C
)

target_link_libraries(test_grid PRIVATE
        OpenMP::OpenMP_C
        MPI::MPI_C
)
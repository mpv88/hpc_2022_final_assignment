# compiler and compilation flags
CC = mpicc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O3 -march=native -fopenmp
CPPFLAGS = -Iinclude

# executable targets
GOL_SERIAL = build/gol_serial
GOL_PARALLEL = build/gol_parallel
PGM_TEST = build/test_pgm
GRID_TEST = build/test_grid
EVOLUTION_COMMON_TEST = build/test_evolution_common

# source files
GOL_SERIAL_SRC = src/main_serial.c src/args.c src/pgm.c src/grid.c src/evolution_common.c src/evolution_ordered.c src/evolution_static.c src/evolution_wave.c src/evolution_wb.c src/benchmark.c
GOL_PARALLEL_SRC = src/main_parallel.c src/args.c src/pgm.c src/grid.c src/evolution_common.c src/evolution_ordered.c src/evolution_static.c src/evolution_wave.c src/evolution_wb.c src/benchmark.c
PGM_TEST_SRC = tests/test_pgm.c src/pgm.c
GRID_TEST_SRC = tests/test_grid.c src/grid.c
EVOLUTION_COMMON_TEST_SRC = tests/test_evolution_common.c src/evolution_common.c

# phony targets and default target
.PHONY: all test_pgm test_grid test_evolution_common clean

all: $(GOL_SERIAL) $(GOL_PARALLEL)

test_pgm: $(PGM_TEST)

test_grid: $(GRID_TEST)

test_evolution_common: $(EVOLUTION_COMMON_TEST)

# main executable targets
$(GOL_SERIAL): $(GOL_SERIAL_SRC) include/args.h include/grid.h include/pgm.h include/evolution_common.h include/evolution_ordered.h include/evolution_static.h include/evolution_wave.h include/evolution_wb.h include/benchmark.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_POSIX_C_SOURCE=200809L $(GOL_SERIAL_SRC) -o $(GOL_SERIAL)

$(GOL_PARALLEL): $(GOL_PARALLEL_SRC) include/args.h include/pgm.h include/grid.h include/evolution_common.h include/evolution_ordered.h include/evolution_static.h include/evolution_wave.h include/evolution_wb.h include/benchmark.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_POSIX_C_SOURCE=200809L $(GOL_PARALLEL_SRC) -o $(GOL_PARALLEL)

# test targets
$(PGM_TEST): $(PGM_TEST_SRC) include/pgm.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_POSIX_C_SOURCE=200809L $(PGM_TEST_SRC) -o $(PGM_TEST)

$(GRID_TEST): $(GRID_TEST_SRC) include/grid.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GRID_TEST_SRC) -o $(GRID_TEST)

$(EVOLUTION_COMMON_TEST): $(EVOLUTION_COMMON_TEST_SRC) include/evolution_common.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) $(EVOLUTION_COMMON_TEST_SRC) -o $(EVOLUTION_COMMON_TEST)

# clean target
clean:
	rm -rf build
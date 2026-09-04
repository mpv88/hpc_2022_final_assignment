CC = mpicc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O3 -march=native -fopenmp
CPPFLAGS = -Iinclude

GOL_SERIAL = build/gol_serial
GOL_PARALLEL = build/gol_parallel
TEST = build/test_pgm
GRID_TEST = build/test_grid

GOL_SERIAL_SRC = src/main_serial.c src/args.c src/pgm.c src/grid.c
GOL_PARALLEL_SRC = src/main_parallel.c src/args.c src/pgm.c src/grid.c
TEST_SRC = tests/test_pgm.c src/pgm.c
GRID_TEST_SRC = tests/test_grid.c src/grid.c

.PHONY: all test test_grid clean

all: $(GOL_SERIAL) $(GOL_PARALLEL)

test: $(TEST)

test_grid: $(GRID_TEST)

$(GOL_SERIAL): $(GOL_SERIAL_SRC) include/args.h include/pgm.h include/grid.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_POSIX_C_SOURCE=200809L $(GOL_SERIAL_SRC) -o $(GOL_SERIAL)

$(GOL_PARALLEL): $(GOL_PARALLEL_SRC) include/args.h include/pgm.h include/grid.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GOL_PARALLEL_SRC) -o $(GOL_PARALLEL)

$(TEST): $(TEST_SRC) include/pgm.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_POSIX_C_SOURCE=200809L $(TEST_SRC) -o $(TEST)

$(GRID_TEST): $(GRID_TEST_SRC) include/grid.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GRID_TEST_SRC) -o $(GRID_TEST)

clean:
	rm -rf build
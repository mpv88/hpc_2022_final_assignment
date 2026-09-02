CC = mpicc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O3 -march=native -fopenmp
CPPFLAGS = -Iinclude

GOL = build/gol
TEST = build/test_pgm
GRID_TEST = build/test_grid

GOL_SRC = src/main.c src/pgm.c src/grid.c
TEST_SRC = tests/test_pgm.c src/pgm.c
GRID_TEST_SRC = tests/test_grid.c src/grid.c

.PHONY: all test test_grid clean

all: $(GOL)

test: $(TEST)

test_grid: $(GRID_TEST)

$(GOL): $(GOL_SRC) include/pgm.h include/grid.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GOL_SRC) -o $(GOL)

$(TEST): $(TEST_SRC) include/pgm.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_POSIX_C_SOURCE=200809L $(TEST_SRC) -o $(TEST)

$(GRID_TEST): $(GRID_TEST_SRC) include/grid.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GRID_TEST_SRC) -o $(GRID_TEST)

clean:
	rm -rf build
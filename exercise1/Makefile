CC = mpicc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O3 -march=native -fopenmp
CPPFLAGS = -Iinclude

GOL = build/gol
TEST = build/test_pgm

GOL_SRC = src/main.c src/pgm.c
TEST_SRC = tests/test_pgm.c src/pgm.c

.PHONY: all test clean

all: $(GOL)

test: $(TEST)

$(GOL): $(GOL_SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GOL_SRC) -o $(GOL)

$(TEST): $(TEST_SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_POSIX_C_SOURCE=200809L $(TEST_SRC) -o $(TEST)

clean:
	rm -rf build
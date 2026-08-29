CC = gcc

CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O3 -march=native -fopenmp

TARGET = build/gol
SRC = src/main.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -rf build
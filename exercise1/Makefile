CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O3 -march=native
TARGET = build/gol
SRC = src/main.c

$(TARGET): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

.PHONY: clean

clean:
	rm -rf build
#include "evolution_common.h"

static int wrap_index(int index, int size)
{
    if (index < 0) return size - 1;
    if (index >= size) return 0;
    return index;
}

int count_live_neighbors(const uint8_t *grid, int row, int column, int width, int height)
{
    int live_neighbors = 0;

    for (int row_offset = -1; row_offset <= 1; row_offset++) {
        for (int column_offset = -1; column_offset <= 1; column_offset++) {
            if (row_offset == 0 && column_offset == 0) continue;

            int neighbor_row = wrap_index(row + row_offset, height);
            int neighbor_column = wrap_index(column + column_offset, width);

            live_neighbors += grid[neighbor_row * width + neighbor_column];
        }
    }
    return live_neighbors;
}

int count_live_neighbors_parallel(const uint8_t *grid, int row, int column, int width)
{
    int live_neighbors = 0;

    for (int row_offset = -1; row_offset <= 1; row_offset++) {
        for (int column_offset = -1; column_offset <= 1; column_offset++) {
            if (row_offset == 0 && column_offset == 0)
                continue;

            int neighbor_row = row + row_offset;
            int neighbor_column = column + column_offset;

            if (neighbor_column < 0)
                neighbor_column = width - 1;
            else if (neighbor_column >= width)
                neighbor_column = 0;

            live_neighbors += grid[neighbor_row * width + neighbor_column];
        }
    }
    return live_neighbors;
}

uint8_t next_cell_state(uint8_t current_state, int live_neighbors)
{
    if (current_state == 1)
        return (live_neighbors == 2 || live_neighbors == 3) ? 1 : 0;
    return live_neighbors == 3 ? 1 : 0;
}
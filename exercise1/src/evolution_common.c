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

void exchange_halos_parallel(uint8_t *grid, int width, int local_rows, int rank, int size, MPI_Comm comm)
{
    int previous_rank = (rank - 1 + size) % size; // previous rank (neighbor above/left in ring)
    int next_rank = (rank + 1) % size; // next rank (neighbor below/right in ring)

    MPI_Request requests[4]; // request handles array for tracking non-blocking ops

    // receive top halo row from previous rank bottom row (tag 0)
    MPI_Irecv(grid - width, width, MPI_UINT8_T, previous_rank, 0, comm, &requests[0]);
    // receive bottom halo row from next rank top row (tag 1)
    MPI_Irecv(grid + (size_t)local_rows * width, width, MPI_UINT8_T, next_rank, 1, comm, &requests[1]);
    // send current rank top row to previous rank bottom halo (tag 1)
    MPI_Isend(grid, width, MPI_UINT8_T, previous_rank, 1, comm, &requests[2]);
    // send current rank bottom row to next rank top halo (tag 0)
    MPI_Isend(grid + (size_t)(local_rows - 1) * width, width, MPI_UINT8_T, next_rank, 0, comm, &requests[3]);

    MPI_Waitall(4, requests, MPI_STATUSES_IGNORE); // wait for all halo exchanges to complete
}
#include "evolution_wb.h"
#include "evolution_common.h"

static void exchange_halos(uint8_t *grid, int width, int local_rows, int rank, int size, MPI_Comm comm)
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

static void update_white_serial(const uint8_t *grid, uint8_t *next_grid, int width, int height)
{
    for (int row = 0; row < height; row++) {
        for (int column = 0; column < width; column++) {
            int index = row * width + column;

            if ((row + column) % 2 == 0) {
                int live_neighbors = count_live_neighbors(grid, row, column, width, height);
                next_grid[index] = next_cell_state(grid[index], live_neighbors);
            } else {
                next_grid[index] = grid[index];
            }
        }
    }
}

static void update_black_serial(const uint8_t *next_grid, uint8_t *grid, int width, int height)
{
    for (int row = 0; row < height; row++) {
        for (int column = 0; column < width; column++) {
            if ((row + column) % 2 != 1)
                continue;

            int index = row * width + column;
            int live_neighbors = count_live_neighbors(next_grid, row, column, width, height);
            grid[index] = next_cell_state(next_grid[index], live_neighbors);
        }
    }
}

static void copy_white_serial(const uint8_t *next_grid, uint8_t *grid, int width, int height)
{
    for (int row = 0; row < height; row++) {
        for (int column = 0; column < width; column++) {
            if ((row + column) % 2 == 0)
                grid[row * width + column] = next_grid[row * width + column];
        }
    }
}

void evolve_wb_serial(uint8_t *grid, uint8_t *next_grid, int width, int height)
{
    // update white cells using the original grid
    update_white_serial(grid, next_grid, width, height);
    // update black cells using the new white cells
    update_black_serial(next_grid, grid, width, height);
    // copy the new white cells into the final grid
    copy_white_serial(next_grid, grid, width, height);
}

static void update_white_parallel(const uint8_t *grid, uint8_t *next_grid, int width, int local_rows, int first_global_row)
{
#pragma omp parallel for schedule(static)
    for (int row = 0; row < local_rows; row++) {
        int global_row = first_global_row + row;

        for (int column = 0; column < width; column++) {
            int index = row * width + column;

            if ((global_row + column) % 2 == 0) {
                int live_neighbors = count_live_neighbors_parallel(grid, row, column, width);
                next_grid[index] = next_cell_state(grid[index], live_neighbors);
            } else {
                next_grid[index] = grid[index];
            }
        }
    }
}

static void update_black_parallel(const uint8_t *next_grid, uint8_t *grid, int width, int local_rows, int first_global_row)
{
#pragma omp parallel for schedule(static)
    for (int row = 0; row < local_rows; row++) {
        int global_row = first_global_row + row;

        for (int column = 0; column < width; column++) {
            if ((global_row + column) % 2 != 1)
                continue;

            int index = row * width + column;
            int live_neighbors = count_live_neighbors_parallel(next_grid, row, column, width);
            grid[index] = next_cell_state(next_grid[index], live_neighbors);
        }
    }
}

static void copy_white_parallel(const uint8_t *next_grid, uint8_t *grid, int width, int local_rows, int first_global_row)
{
#pragma omp parallel for schedule(static)
    for (int row = 0; row < local_rows; row++) {
        int global_row = first_global_row + row;

        for (int column = 0; column < width; column++) {
            if ((global_row + column) % 2 == 0)
                grid[row * width + column] = next_grid[row * width + column];
        }
    }
}

void evolve_wb_parallel(uint8_t *grid, uint8_t *next_grid, int width, int height, int local_rows, int rank, int size, MPI_Comm comm)
{
    int base_rows = height / size;
    int remainder = height % size;
    int first_global_row = rank * base_rows + (rank < remainder ? rank : remainder);

    // exchange the old halo before updating white cells
    exchange_halos(grid, width, local_rows, rank, size, comm);
    // update white cells using the original grid
    update_white_parallel(grid, next_grid, width, local_rows, first_global_row);
    // exchange the updated white boundary
    exchange_halos(next_grid, width, local_rows, rank, size, comm);
    // update black cells using the new white cells
    update_black_parallel(next_grid, grid, width, local_rows, first_global_row);
    // copy the new white cells into the final grid
    copy_white_parallel(next_grid, grid, width, local_rows, first_global_row);
}
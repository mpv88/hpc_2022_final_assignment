#include "evolution_wave.h"
#include "evolution_common.h"

#include <stdlib.h>

static int periodic_distance(int first, int second, int size)
{
    int distance = abs(first - second);
    int wrapped_distance = size - distance;
    // shortest periodic distance between two indices on a ring
    return distance < wrapped_distance ? distance : wrapped_distance;
}

static int chebyshev_distance(int row, int column, int start_row, int start_column, int width, int height)
{
    int row_distance = periodic_distance(row, start_row, height);
    int column_distance = periodic_distance(column, start_column, width);
    // max periodic Chebyshev distance between two cells
    return row_distance > column_distance ? row_distance : column_distance;
}

void evolve_wave_serial(uint8_t *grid, uint8_t *next_grid, int width, int height, int start_row, int start_column)
{
    int max_distance = (width > height ? width : height) / 2; // max Chebyshev distance

    for (int distance = 0; distance <= max_distance; distance++) {
        // compute the current wavefront from grid into next_grid
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                if (chebyshev_distance(row, column, start_row, start_column, width, height) != distance)
                    continue;

                int index = row * width + column;
                int live_neighbors = count_live_neighbors(grid, row, column, width, height);
                next_grid[index] = next_cell_state(grid[index], live_neighbors);
            }
        }

        // apply the computed wavefront from next_grid back into grid
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                if (chebyshev_distance(row, column, start_row, start_column, width, height) != distance)
                    continue;

                int index = row * width + column;
                grid[index] = next_grid[index];
            }
        }
    }
}

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

void evolve_wave_parallel(uint8_t *grid, uint8_t *next_grid, int width, int height, int local_rows, int rank, int size, int start_row, int start_column, MPI_Comm comm)
{
    int base_rows = height / size; // base #rows per rank
    int remainder = height % size; // #ranks receiving additional row(s)
    int first_global_row = rank * base_rows + (rank < remainder ? rank : remainder);
    int max_distance = (width > height ? width : height) / 2; // max Chebyshev distance

    // exchange the initial grid boundary
    exchange_halos(grid, width, local_rows, rank, size, comm);

    for (int distance = 0; distance <= max_distance; distance++) {
        // compute the current local wavefront from grid into next_grid
#pragma omp parallel for schedule(static)
        for (int local_row = 0; local_row < local_rows; local_row++) {
            int global_row = first_global_row + local_row;

            for (int column = 0; column < width; column++) {
                if (chebyshev_distance(global_row, column, start_row, start_column, width, height) != distance)
                    continue;

                int index = local_row * width + column;
                int live_neighbors = count_live_neighbors_parallel(grid, local_row, column, width);
                next_grid[index] = next_cell_state(grid[index], live_neighbors);
            }
        }

        // apply the computed local wavefront from next_grid back into grid
#pragma omp parallel for schedule(static)
        for (int local_row = 0; local_row < local_rows; local_row++) {
            int global_row = first_global_row + local_row; // convert local row index to global row index

            for (int column = 0; column < width; column++) {
                if (chebyshev_distance(global_row, column, start_row, start_column, width, height) != distance)
                    continue;

                int index = local_row * width + column;
                grid[index] = next_grid[index];
            }
        }
        // exchange updated boundary rows for the next wavefront
        exchange_halos(grid, width, local_rows, rank, size, comm);
        // wait until all ranks complete the current wavefront
        MPI_Barrier(comm);
    }
}
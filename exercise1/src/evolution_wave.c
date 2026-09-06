#include "evolution_wave.h"
#include "evolution_common.h"

#include <stdlib.h>

static int periodic_distance(int first, int second, int size)
{
    int distance = abs(first - second);
    int wrapped_distance = size - distance;

    return distance < wrapped_distance ? distance : wrapped_distance;
}

static int chebyshev_distance(int row, int column, int start_row, int start_column, int width, int height)
{
    int row_distance = periodic_distance(row, start_row, height);
    int column_distance = periodic_distance(column, start_column, width);

    return row_distance > column_distance ? row_distance : column_distance;
}

void evolve_wave_serial(const uint8_t *current, uint8_t *next, int width, int height, int start_row, int start_column)
{   // max Chebyshev distance needed to cover the whole domain
    int max_distance = (width > height ? width : height) / 2;
    // process wavefronts in increasing Chebyshev distance from chosen centre
    for (int distance = 0; distance <= max_distance; distance++) {
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                // only update cells belonging to the current wavefront
                if (chebyshev_distance(row, column, start_row, start_column, width, height) != distance)
                    continue;

                int live_neighbors = count_live_neighbors(current, row, column, width, height);
                next[row * width + column] = next_cell_state(current[row * width + column], live_neighbors);
            }
        }
    }
}

void evolve_wave_parallel(uint8_t *current, uint8_t *next, int width, int height, int local_rows, int rank, int size, int start_row, int start_column, MPI_Comm comm)
{
    int previous_rank = (rank - 1 + size) % size; // previous rank (neighbor above/left in ring)
    int next_rank = (rank + 1) % size; // next rank (neighbor below/right in ring)

    int base_rows = height / size; // grid split evenly with first (height % size) ranks each get extra row
    int first_global_row = rank * base_rows + (rank < height % size ? rank : height % size);
    int max_distance = (width > height ? width : height) / 2; // conservative upper bound on Chebyshev distance

    MPI_Request requests[4]; // request handles array for tracking non-blocking ops

    // receive top halo row from previous rank bottom row (tag 0)
    MPI_Irecv(current - width, width, MPI_UINT8_T, previous_rank, 0, comm, &requests[0]);
    // receive bottom halo row from next rank top row (tag 1)
    MPI_Irecv(current + (size_t)local_rows * width, width, MPI_UINT8_T, next_rank, 1, comm, &requests[1]);
    // send current rank top row to previous rank bottom halo (tag 1)
    MPI_Isend(current, width, MPI_UINT8_T, previous_rank, 1, comm, &requests[2]);
    // send current rank bottom row to next rank top halo (tag 0)
    MPI_Isend(current + (size_t)(local_rows - 1) * width, width, MPI_UINT8_T, next_rank, 0, comm, &requests[3]);

    MPI_Waitall(4, requests, MPI_STATUSES_IGNORE); // wait for all halo exchanges to complete

    for (int distance = 0; distance <= max_distance; distance++) {
#pragma omp parallel for schedule(static)
        for (int local_row = 0; local_row < local_rows; local_row++) {
            int global_row = first_global_row + local_row;

            for (int column = 0; column < width; column++) {
                if (chebyshev_distance(global_row, column, start_row, start_column, width, height) != distance)
                    continue; // skip cells not belonging to the current wavefront layer

                int live_neighbors = count_live_neighbors_parallel(current, local_row, column, width);
                next[local_row * width + column] = next_cell_state(current[local_row * width + column], live_neighbors);
            }
        }

        // ensure complete wavefront is finished before starting next
        MPI_Barrier(comm);
    }
}
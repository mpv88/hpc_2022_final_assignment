#include "evolution_static.h"
#include "evolution_common.h"

void evolve_static_serial(const uint8_t *current, uint8_t *next, int width, int height)
{
    for (int row = 0; row < height; row++) {
        for (int column = 0; column < width; column++) {
            int live_neighbors = count_live_neighbors(current, row, column, width, height);
            next[row * width + column] = next_cell_state(current[row * width + column], live_neighbors);
        }
    }
}

void evolve_static_parallel(uint8_t *current, uint8_t *next, int width, int local_rows, int rank, int size, MPI_Comm comm)
{
    int previous_rank = (rank - 1 + size) % size; // previous rank (neighbor above/left in ring)
    int next_rank = (rank + 1) % size; // next rank (neighbor below/right in ring)

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

#pragma omp parallel for schedule(static)
    for (int row = 0; row < local_rows; row++) {
        for (int column = 0; column < width; column++) {
            int live_neighbors = count_live_neighbors_parallel(current, row, column, width);
            next[row * width + column] = next_cell_state(current[row * width + column], live_neighbors);
        }
    }
}
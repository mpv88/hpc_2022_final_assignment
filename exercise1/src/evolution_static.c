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
    exchange_halos_parallel(current, width, local_rows, rank, size, comm);

#pragma omp parallel for schedule(static)
    for (int row = 0; row < local_rows; row++) {
        for (int column = 0; column < width; column++) {
            int live_neighbors = count_live_neighbors_parallel(current, row, column, width);
            next[row * width + column] = next_cell_state(current[row * width + column], live_neighbors);
        }
    }
}
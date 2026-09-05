#include "evolution_ordered.h"
#include "evolution_common.h"

static void update_cell_ordered(uint8_t *grid, int row, int column, int width, int height)
{
    int live_neighbors = count_live_neighbors(grid, row, column, width, height);
    grid[row * width + column] = next_cell_state(grid[row * width + column], live_neighbors);
}

static void update_cell_ordered_parallel(uint8_t *grid, int row, int column, int width)
{
    int live_neighbors = count_live_neighbors_parallel(grid, row, column, width);
    grid[row * width + column] = next_cell_state(grid[row * width + column], live_neighbors);
}

void evolve_ordered_serial(uint8_t *grid, int width, int height)
{
    for (int row = 0; row < height; row++) {
        for (int column = 0; column < width; column++)
            update_cell_ordered(grid, row, column, width, height);
    }
}

void evolve_ordered_parallel(uint8_t *grid, int width, int local_rows, int rank, int size, MPI_Comm comm)
{
    if (size == 1) {
        for (int row = 0; row < local_rows; row++) {
            for (int column = 0; column < width; column++)
                update_cell_ordered_parallel(grid, row, column, width);
        }
        return;
    }

    int previous_rank = (rank - 1 + size) % size;
    int next_rank = (rank + 1) % size;

    //exchange old boundary rows
    MPI_Sendrecv(grid, width, MPI_UINT8_T, previous_rank, 0,
                 grid + local_rows * width, width, MPI_UINT8_T, next_rank, 0,
                 comm, MPI_STATUS_IGNORE);

    MPI_Sendrecv(grid + (local_rows - 1) * width, width, MPI_UINT8_T, next_rank, 1,
                 grid - width, width, MPI_UINT8_T, previous_rank, 1,
                 comm, MPI_STATUS_IGNORE);

    if (rank == 0) {
        //rank 0 updates first in ordered sequence
        for (int row = 0; row < local_rows; row++) {
            for (int column = 0; column < width; column++)
                update_cell_ordered_parallel(grid, row, column, width);
        }

        //send updated last row to rank 1
        MPI_Send(grid + (local_rows - 1) * width, width, MPI_UINT8_T, 1, 2, comm);

        //send updated first row to the last rank
        MPI_Send(grid, width, MPI_UINT8_T, size - 1, 3, comm);
    } else {
        //receive updated last row from previous rank
        MPI_Recv(grid - width, width, MPI_UINT8_T, previous_rank, 2, comm, MPI_STATUS_IGNORE);

        if (rank < size - 1) {
            //intermediate rank updates after previous rank
            for (int row = 0; row < local_rows; row++) {
                for (int column = 0; column < width; column++)
                    update_cell_ordered_parallel(grid, row, column, width);
            }

            //send updated last row to next rank
            MPI_Send(grid + (local_rows - 1) * width, width, MPI_UINT8_T, next_rank, 2, comm);
        } else {
            //last rank updates all rows except its final row
            for (int row = 0; row < local_rows - 1; row++) {
                for (int column = 0; column < width; column++)
                    update_cell_ordered_parallel(grid, row, column, width);
            }

            //receive rank 0's updated first row
            MPI_Recv(grid + local_rows * width, width, MPI_UINT8_T, 0, 3, comm, MPI_STATUS_IGNORE);

            //update final row
            for (int column = 0; column < width; column++)
                update_cell_ordered_parallel(grid, local_rows - 1, column, width);
        }
    }
}
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
    if (size == 1) { // single-rank case no communication, just update all rows
        for (int row = 0; row < local_rows; row++) {
            for (int column = 0; column < width; column++)
                update_cell_ordered_parallel(grid, row, column, width);
        }
        return;
    }

    int previous_rank = (rank - 1 + size) % size; // previous rank (neighbor above/left in ring)
    int next_rank = (rank + 1) % size; // next rank (neighbor below/right in ring)

    //exchange old boundary rows
    MPI_Sendrecv(grid, width, MPI_UINT8_T, previous_rank, 0,
                 grid + local_rows * width, width, MPI_UINT8_T, next_rank, 0,
                 comm, MPI_STATUS_IGNORE); // send current rank first row to previous / receive bottom halo from next (tag 0)

    MPI_Sendrecv(grid + (local_rows - 1) * width, width, MPI_UINT8_T, next_rank, 1,
                 grid - width, width, MPI_UINT8_T, previous_rank, 1,
                 comm, MPI_STATUS_IGNORE); // send current rank last row to next / receive top halo from previous (tag 1)

    if (rank == 0) {
        // rank 0 updates first in ordered sequence
        for (int row = 0; row < local_rows; row++) {
            for (int column = 0; column < width; column++)
                update_cell_ordered_parallel(grid, row, column, width);
        }

        // send updated last row to rank 1 (tag 2)
        MPI_Send(grid + (local_rows - 1) * width, width, MPI_UINT8_T, 1, 2, comm);

        // send updated first row to the last rank (tag 3)
        MPI_Send(grid, width, MPI_UINT8_T, size - 1, 3, comm);
    } else {
        //receive updated last row from previous rank into top halo (tag 2)
        MPI_Recv(grid - width, width, MPI_UINT8_T, previous_rank, 2, comm, MPI_STATUS_IGNORE);

        if (rank < size - 1) {
            // intermediate rank updates all rows using fresh top halo
            for (int row = 0; row < local_rows; row++) {
                for (int column = 0; column < width; column++)
                    update_cell_ordered_parallel(grid, row, column, width);
            }

            //send updated last row to next rank (tag 2)
            MPI_Send(grid + (local_rows - 1) * width, width, MPI_UINT8_T, next_rank, 2, comm);
        } else {
            // last rank update all but the final row using fresh top halo
            for (int row = 0; row < local_rows - 1; row++) {
                for (int column = 0; column < width; column++)
                    update_cell_ordered_parallel(grid, row, column, width);
            }

            // receive rank 0 updated first row into bottom halo (tag 3)
            MPI_Recv(grid + local_rows * width, width, MPI_UINT8_T, 0, 3, comm, MPI_STATUS_IGNORE);

            // update final row using the fresh bottom halo from rank 0
            for (int column = 0; column < width; column++)
                update_cell_ordered_parallel(grid, local_rows - 1, column, width);
        }
    }
}
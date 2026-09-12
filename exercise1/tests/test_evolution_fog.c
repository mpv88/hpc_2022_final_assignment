#include "evolution_fog.h"
#include "evolution_common.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

static int count_live_cells(const uint8_t *grid, int width, int height)
{
    int count = 0;

    for (int row = 0; row < height; row++) {
        for (int column = 0; column < width; column++)
            count += grid[row * width + column];
    }

    return count;
}

static int get_local_start_row(int height, int size, int rank)
{
    int base_rows = height / size;
    int remainder = height % size;

    return rank * base_rows + (rank < remainder ? rank : remainder);
}

static void initialize_parallel_grid(uint8_t *grid, int width, int height,
                                     int local_rows, int start_row, int rank,
                                     int size, const uint8_t *global_grid)
{
    for (int row = 0; row < local_rows; row++) {
        for (int column = 0; column < width; column++)
            grid[row * width + column] = global_grid[(start_row + row) * width + column];
    }

    int previous_row = (start_row - 1 + height) % height;
    int next_row = (start_row + local_rows) % height;

    for (int column = 0; column < width; column++) {
        grid[-width + column] = global_grid[previous_row * width + column];
        grid[local_rows * width + column] = global_grid[next_row * width + column];
    }

    (void)rank;
    (void)size;
}

static void gather_parallel_grid(const uint8_t *grid, int width, int height,
                                 int local_rows, int rank, int size,
                                 uint8_t *global_grid)
{
    int *row_counts = NULL;
    int *displacements = NULL;

    if (rank == 0) {
        row_counts = malloc((size_t)size * sizeof(int));
        displacements = malloc((size_t)size * sizeof(int));
        assert(row_counts != NULL);
        assert(displacements != NULL);

        int offset = 0;

        for (int process = 0; process < size; process++) {
            int base_rows = height / size;
            int remainder = height % size;
            int rows = base_rows + (process < remainder);

            row_counts[process] = rows * width;
            displacements[process] = offset;
            offset += rows * width;
        }
    }

    MPI_Gatherv(grid, local_rows * width, MPI_UINT8_T,
                global_grid, row_counts, displacements,
                MPI_UINT8_T, 0, MPI_COMM_WORLD);

    free(row_counts);
    free(displacements);
}

static void test_fog_death(void)
{
    const int width = 5;
    const int height = 5;
    uint8_t grid[25];

    memset(grid, 1, sizeof(grid));

    fog_seed(1);
    apply_fog_serial(grid, width, height, 0.0);

    assert(count_live_cells(grid, width, height) == 24);
}

static void test_fog_life_and_spreading(void)
{
    const int width = 5;
    const int height = 5;
    uint8_t grid[25];

    memset(grid, 0, sizeof(grid));

    fog_seed(1);
    apply_fog_serial(grid, width, height, 1.0);

    assert(count_live_cells(grid, width, height) == 3);
}

static void test_fog_no_spreading_when_enough_neighbors(void)
{
    const int width = 5;
    const int height = 5;
    uint8_t grid[25];

    memset(grid, 1, sizeof(grid));

    fog_seed(1);
    apply_fog_serial(grid, width, height, 1.0);

    assert(count_live_cells(grid, width, height) == 25);
}

static void test_fog_reproducibility(void)
{
    const int width = 5;
    const int height = 5;
    uint8_t grid1[25];
    uint8_t grid2[25];

    memset(grid1, 0, sizeof(grid1));
    memset(grid2, 0, sizeof(grid2));

    fog_seed(12345);
    apply_fog_serial(grid1, width, height, 1.0);

    fog_seed(12345);
    apply_fog_serial(grid2, width, height, 1.0);

    assert(memcmp(grid1, grid2, sizeof(grid1)) == 0);
}

static void test_fog_parallel_matches_serial(int rank, int size)
{
    const int width = 5;
    const int height = 5;

    uint8_t global_initial[25];
    uint8_t serial_grid[25];
    uint8_t *parallel_allocation;
    uint8_t *parallel_grid;
    uint8_t *parallel_result = NULL;

    memset(global_initial, 0, sizeof(global_initial));

    // create a small pattern with both live and dead cells
    global_initial[1 * width + 1] = 1;
    global_initial[1 * width + 2] = 1;
    global_initial[2 * width + 1] = 1;
    global_initial[3 * width + 3] = 1;
    global_initial[4 * width + 4] = 1;

    memcpy(serial_grid, global_initial, sizeof(serial_grid));

    int local_rows = height / size + (rank < height % size);
    int local_start_row = get_local_start_row(height, size, rank);

    parallel_allocation = malloc((size_t)(local_rows + 2) * width);
    assert(parallel_allocation != NULL);

    parallel_grid = parallel_allocation + width;

    initialize_parallel_grid(parallel_grid, width, height, local_rows,
                              local_start_row, rank, size, global_initial);

    fog_seed(12345);
    apply_fog_serial(serial_grid, width, height, 1.0);

    if (rank == 0) {
        parallel_result = malloc((size_t)width * height);
        assert(parallel_result != NULL);
    }

    fog_seed(12345);

    // refresh halos before applying the global FoG event
    exchange_halos_parallel(parallel_grid, width, local_rows,
                            rank, size, MPI_COMM_WORLD);

    apply_fog_parallel(parallel_grid, width, height, local_rows,
                       local_start_row, rank, size, 1.0,
                       MPI_COMM_WORLD);

    gather_parallel_grid(parallel_grid, width, height, local_rows,
                         rank, size, parallel_result);

    if (rank == 0)
        assert(memcmp(serial_grid, parallel_result, sizeof(serial_grid)) == 0);

    free(parallel_result);
    free(parallel_allocation);
}

int main(int argc, char **argv)
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        test_fog_death();
        test_fog_life_and_spreading();
        test_fog_no_spreading_when_enough_neighbors();
        test_fog_reproducibility();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    test_fog_parallel_matches_serial(rank, size);

    if (rank == 0)
        printf("all evolution_fog tests passed\n");

    MPI_Finalize();
    return 0;
}
#include "../include/grid.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int width = 100;
    const int height = 100;
    const unsigned int seed = 12345;

    if (rank == 0)
        printf("grid initialization test\n");

    uint8_t *data = NULL;

    if (rank == 0) {
        printf("size: %d x %d\n", width, height);
        printf("seed: %u\n\n", seed);

        uint8_t *serial_data = NULL;

        int result = grid_initialize(&serial_data, width, height, seed);
        if (result != 0) {
            printf("FAIL: serial grid_initialize()\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("serial initialization: OK\n");

        size_t total_size = (size_t)width * (size_t)height;
        for (size_t i = 0; i < total_size; i++) {
            if (serial_data[i] != 0 && serial_data[i] != 1) {
                printf("FAIL: invalid serial cell value at index %zu\n", i);
                free(serial_data);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }

        printf("serial cell values:    OK\n");
        free(serial_data);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    int local_rows = 0;

    int result = grid_initialize_mpi(&data, width, height, &local_rows, rank, size, seed);

    if (result != 0) {
        printf("FAIL: MPI rank %d grid initialization\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int expected_rows = height / size + (rank < height % size);

    if (local_rows != expected_rows) {
        printf("FAIL: MPI rank %d received %d rows, expected %d\n", rank, local_rows, expected_rows);
        free(data);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    size_t local_size = (size_t)width * (size_t)local_rows;

    for (size_t i = 0; i < local_size; i++) {
        if (data[i] != 0 && data[i] != 1) {
            printf("FAIL: invalid cell value on MPI rank %d at index %zu\n", rank, i);
            free(data);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    if (rank == 0)
        printf("\nparallel initialization: OK\n");

    printf("MPI rank %d: %d local rows, cell values OK\n", rank, local_rows);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0)
        printf("\nALL GRID TESTS PASSED\n");

    MPI_Finalize();
    return 0;
}
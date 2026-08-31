#include "../include/pgm.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CPU_TIME (clock_gettime(CLOCK_MONOTONIC, &ts), (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9)

int main(void)
{
    int rank, size;
    struct timespec ts;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const char *input = "patterns/validate/pgm/blinker_00000.pgm";
    const char *output = "tests/images/blinker_test.pgm";
    const char *mpi_output = "tests/images/blinker_mpi_test.pgm";

    if (rank == 0) {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("working directory: %s\n", cwd);
        printf("MPI ranks: %d\n", size);
        printf("input: %s\n\n", input);

        uint8_t *data = NULL;
        int width, height;
        double tstart, time;
        // serial read
        tstart = CPU_TIME;
        int result = pgm_read(input, &data, &width, &height);
        time = CPU_TIME - tstart;

        if (result != 0) {
            printf("pgm_read failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("serial read:  %f s (%d x %d)\n", time, width, height);
        // serial write
        tstart = CPU_TIME;
        result = pgm_write(output, data, width, height);
        time = CPU_TIME - tstart;

        if (result != 0) {
            printf("pgm_write failed\n");
            free(data);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("serial write: %f s\n", time);
        printf("written: %s\n\n", output);

        free(data);
    }
    // parallel read
    uint8_t *local_data = NULL;
    int local_width, local_rows, global_height;

    MPI_Barrier(MPI_COMM_WORLD);
    double tstart = MPI_Wtime();

    int result = pgm_read_mpi(input, &local_data, &local_width, &local_rows,
                              &global_height, rank, size);

    MPI_Barrier(MPI_COMM_WORLD);
    double time = MPI_Wtime() - tstart;

    if (result != 0) {
        printf("rank %d: pgm_read_mpi failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0)
        printf("MPI read:     %f s (%d x %d, %d ranks)\n",
               time, local_width, global_height, size);
    // serial write from parallel read
    int *recv_counts = NULL;
    int *displacements = NULL;
    uint8_t *global_data = NULL;

    if (rank == 0) {
        recv_counts = malloc(size * sizeof(int));
        displacements = malloc(size * sizeof(int));
        global_data = malloc((size_t)local_width * global_height);

        if (recv_counts == NULL || displacements == NULL || global_data == NULL) {
            printf("MPI test allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    int local_size = local_rows * local_width;
    MPI_Gather(&local_size, 1, MPI_INT, recv_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        displacements[0] = 0;
        for (int i = 1; i < size; i++)
            displacements[i] = displacements[i - 1] + recv_counts[i - 1];
    }

    MPI_Gatherv(local_data, local_size, MPI_UNSIGNED_CHAR,
                global_data, recv_counts, displacements, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        result = pgm_write(mpi_output, global_data, local_width, global_height);

        if (result != 0) {
            printf("MPI test write failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("MPI reconstructed image: %s\n", mpi_output);

        free(recv_counts);
        free(displacements);
        free(global_data);
    }

    free(local_data);
    MPI_Finalize();
    return 0;
}
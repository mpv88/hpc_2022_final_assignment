#include "pgm.h"
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
    const char *serial_output = "tests/images/blinker_serial_test.pgm";
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
        result = pgm_write(serial_output, data, width, height);
        time = CPU_TIME - tstart;

        if (result != 0) {
            printf("pgm_write failed\n");
            free(data);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("serial write: %f s\n", time);
        printf("written: %s\n\n", serial_output);

        free(data);
    }

    // parallel read
    uint8_t *local_data = NULL;
    int width, local_rows, height;

    MPI_Barrier(MPI_COMM_WORLD);
    double tstart = MPI_Wtime();

    int result = pgm_read_mpi(input, &local_data, &width, &local_rows,
                              &height, rank, size);

    MPI_Barrier(MPI_COMM_WORLD);
    double time = MPI_Wtime() - tstart;

    if (result != 0) {
        printf("rank %d: pgm_read_mpi failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0)
        printf("MPI read:     %f s (%d x %d, %d ranks)\n",
               time, width, height, size);

    // parallel write
    MPI_Barrier(MPI_COMM_WORLD);
    tstart = MPI_Wtime();

    result = pgm_write_mpi(mpi_output, local_data, width, local_rows,
                           height, rank, size);

    MPI_Barrier(MPI_COMM_WORLD);
    time = MPI_Wtime() - tstart;

    if (result != 0) {
        printf("rank %d: pgm_write_mpi failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0) {
        printf("MPI write:    %f s\n", time);
        printf("written: %s\n", mpi_output);
    }

    free(local_data - width);
    MPI_Finalize();
    return 0;
}
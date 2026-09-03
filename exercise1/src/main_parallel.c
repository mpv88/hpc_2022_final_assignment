#include "args.h"
#include "grid.h"
#include "pgm.h"

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/// calculates the elapsed time in seconds between two timestamps
static double elapsed_time(double start, double end)
{
    return end - start;
}

int main(int argc, char **argv)
{
    arguments_t args;
    uint8_t *grid = NULL;
    int rank, size;
    int width, height, local_rows;
    double start, end;
    double initialization_time = 0.0;
    double read_time = 0.0;
    double write_time = 0.0;
    double evolution_time = 0.0;

    // initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (parse_arguments(argc, argv, &args) != 0) {
        MPI_Finalize();
        return 1;
    }

    if (args.action == INIT) {
        width = args.width;
        height = args.height;

        start = MPI_Wtime();

        if (grid_initialize_mpi(&grid, width, height, &local_rows, rank, size, 1) != 0) {
            free_arguments(&args);
            MPI_Finalize();
            return 1;
        }

        end = MPI_Wtime();
        initialization_time = elapsed_time(start, end);

        start = MPI_Wtime();

        if (pgm_write_mpi(args.filename, grid, width, local_rows, height, rank, size) != 0) {
            free(grid);
            free_arguments(&args);
            MPI_Finalize();
            return 1;
        }

        end = MPI_Wtime();
        write_time = elapsed_time(start, end);

        if (rank == 0)
            printf("initialization_time=%.6f write_time=%.6f\n", initialization_time, write_time);

        free(grid);
    } else if (args.action == RUN) {
        start = MPI_Wtime();

        if (pgm_read_mpi(args.filename, &grid, &width, &local_rows, &height, rank, size) != 0) {
            free_arguments(&args);
            MPI_Finalize();
            return 1;
        }

        end = MPI_Wtime();
        read_time = elapsed_time(start, end);

        /* TODO:
         * evolve the local grid for args.steps steps using args.evolution
         * exchange boundary rows between MPI processes when required
         * measure the evolution time
         * write snapshots according to args.dump_frequency
         */

        if (rank == 0)
            printf("read_time=%.6f evolution_time=%.6f\n", read_time, evolution_time);

        free(grid);
    }

    free_arguments(&args);
    MPI_Finalize();
    return 0;
}
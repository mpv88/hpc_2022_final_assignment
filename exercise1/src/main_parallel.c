#include "args.h"
#include "grid.h"
#include "pgm.h"
#include "evolution_ordered.h"
#include "evolution_static.h"

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

// calculates the elapsed time in seconds between two timestamps
static double elapsed_time(double start, double end)
{
    return end - start;
}

static int write_snapshot(const char *pattern_name, const uint8_t *grid, int width, int local_rows, int height, int rank, int size, int step)
{
    char *filename = build_snapshot_filename(pattern_name, step);

    if (filename == NULL)
        return -1;

    int result = pgm_write_mpi(filename, grid, width, local_rows, height, rank, size);
    free(filename);
    return result;
}

int main(int argc, char **argv)
{
    arguments_t args;
    uint8_t *grid = NULL;
    uint8_t *next_grid = NULL;
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

        char *filename = build_snapshot_filename(args.pattern_name, 0);

        if (filename == NULL) {
            free_arguments(&args);
            MPI_Finalize();
            return 1;
        }

        MPI_Barrier(MPI_COMM_WORLD);
        start = MPI_Wtime();

        if (grid_initialize_mpi(&grid, width, height, &local_rows, rank, size, INITIALIZATION_SEED) != 0) {
            free(filename);
            free_arguments(&args);
            MPI_Finalize();
            return 1;
        }

        MPI_Barrier(MPI_COMM_WORLD);
        end = MPI_Wtime();
        initialization_time = elapsed_time(start, end);

        MPI_Barrier(MPI_COMM_WORLD);
        start = MPI_Wtime();

        if (pgm_write_mpi(filename, grid, width, local_rows, height, rank, size) != 0) {
            free(grid - width);
            free(filename);
            free_arguments(&args);
            MPI_Finalize();
            return 1;
        }

        MPI_Barrier(MPI_COMM_WORLD);
        end = MPI_Wtime();
        write_time = elapsed_time(start, end);

        if (rank == 0)
            printf("initialization_time=%.6f write_time=%.6f\n", initialization_time, write_time);

        free(grid - width);
        free(filename);
    } else if (args.action == RUN) {
        char *filename = build_snapshot_filename(args.pattern_name, 0);

        if (filename == NULL) {
            free_arguments(&args);
            MPI_Finalize();
            return 1;
        }

        MPI_Barrier(MPI_COMM_WORLD);
        start = MPI_Wtime();

        if (pgm_read_mpi(filename, &grid, &width, &local_rows, &height, rank, size) != 0) {
            free(filename);
            free_arguments(&args);
            MPI_Finalize();
            return 1;
        }

        MPI_Barrier(MPI_COMM_WORLD);
        end = MPI_Wtime();
        read_time = elapsed_time(start, end);

        if (args.evolution == STATIC) {
            size_t allocation_size = ((size_t)local_rows + 2) * (size_t)width;
            uint8_t *allocation = malloc(allocation_size);

            if (allocation == NULL) {
                free(grid - width);
                free(filename);
                free_arguments(&args);
                MPI_Finalize();
                return 1;
            }

            next_grid = allocation + width;
        }

        for (int step = 1; step <= args.steps; step++) {
            MPI_Barrier(MPI_COMM_WORLD);
            start = MPI_Wtime();

            if (args.evolution == ORDERED) {
                evolve_ordered_parallel(grid, width, local_rows, rank, size, MPI_COMM_WORLD);
            } else if (args.evolution == STATIC) {
                evolve_static_parallel(grid, next_grid, width, local_rows, rank, size, MPI_COMM_WORLD);

                uint8_t *temporary = grid;
                grid = next_grid;
                next_grid = temporary;
            } else {
                if (rank == 0)
                    fprintf(stderr, "evolution type not implemented yet\n");

                free(next_grid != NULL ? next_grid - width : NULL);
                free(grid - width);
                free(filename);
                free_arguments(&args);
                MPI_Finalize();
                return 1;
            }

            MPI_Barrier(MPI_COMM_WORLD);
            end = MPI_Wtime();
            evolution_time += elapsed_time(start, end);

            if (args.dump_frequency > 0 && step % args.dump_frequency == 0) {
                if (write_snapshot(args.pattern_name, grid, width, local_rows, height, rank, size, step) != 0) {
                    free(next_grid != NULL ? next_grid - width : NULL);
                    free(grid - width);
                    free(filename);
                    free_arguments(&args);
                    MPI_Finalize();
                    return 1;
                }
            }
        }

        if (args.dump_frequency == 0) {
            if (write_snapshot(args.pattern_name, grid, width, local_rows, height, rank, size, args.steps) != 0) {
                free(next_grid != NULL ? next_grid - width : NULL);
                free(grid - width);
                free(filename);
                free_arguments(&args);
                MPI_Finalize();
                return 1;
            }
        }

        if (rank == 0)
            printf("read_time=%.6f evolution_time=%.6f\n", read_time, evolution_time);

        free(next_grid != NULL ? next_grid - width : NULL);
        free(grid - width);
        free(filename);
    }

    free_arguments(&args);
    MPI_Finalize();
    return 0;
}
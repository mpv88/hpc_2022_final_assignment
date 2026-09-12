#include "args.h"
#include "grid.h"
#include "pgm.h"
#include "benchmark.h"
#include "evolution_ordered.h"
#include "evolution_static.h"
#include "evolution_wave.h"
#include "evolution_wb.h"
#include "evolution_fog.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// calculates the elapsed time in seconds between two timestamps
static double elapsed_time(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;
}

static int write_snapshot(const char *pattern_name, const uint8_t *grid, int width, int height, int step)
{
    char *filename = build_snapshot_filename(pattern_name, step);

    if (filename == NULL)
        return -1;

    int result = pgm_write(filename, grid, width, height);
    free(filename);
    return result;
}

int main(int argc, char **argv)
{
    arguments_t args;
    uint8_t *grid = NULL;
    uint8_t *next_grid = NULL;
    struct timespec start, end;
    double initialization_time = 0.0;
    double read_time = 0.0;
    double write_time = 0.0;
    double evolution_time = 0.0;

    if (parse_arguments(argc, argv, &args) != 0) return 1;

    if (args.action == INIT) {
        char *filename = build_snapshot_filename(args.pattern_name, 0);

        if (filename == NULL) {
            free_arguments(&args);
            return 1;
        }

        // init grid
        clock_gettime(CLOCK_MONOTONIC, &start);

        if (grid_initialize(&grid, args.width, args.height, INITIALIZATION_SEED) != 0) {
            free(filename);
            free_arguments(&args);
            return 1;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        initialization_time = elapsed_time(start, end);

        // write pgm
        clock_gettime(CLOCK_MONOTONIC, &start);

        if (pgm_write(filename, grid, args.width, args.height) != 0) {
            free(grid);
            free(filename);
            free_arguments(&args);
            return 1;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        write_time = elapsed_time(start, end);

        printf("initialization_time=%.6f write_time=%.6f\n", initialization_time, write_time);

        free(grid);
        free(filename);
    } else if (args.action == RUN) {
        int width;
        int height;
        int start_row = 0;
        int start_column = 0;
        char *filename = build_snapshot_filename(args.pattern_name, 0);

        if (filename == NULL) {
            free_arguments(&args);
            return 1;
        }

        // read pgm
        clock_gettime(CLOCK_MONOTONIC, &start);

        if (pgm_read(filename, &grid, &width, &height) != 0) {
            free(filename);
            free_arguments(&args);
            return 1;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        read_time = elapsed_time(start, end);

        if (args.evolution == STATIC || args.evolution == WAVE || args.evolution == WHITE_BLACK) {
            next_grid = malloc((size_t)width * (size_t)height);

            if (next_grid == NULL) {
                free(grid);
                free(filename);
                free_arguments(&args);
                return 1;
            }
        }

        if (args.evolution == WAVE)
            srand(INITIALIZATION_SEED);

        if (args.fog_enabled)
            fog_seed(FOG_SEED);

        // evolution
        for (int step = 1; step <= args.steps; step++) {
            clock_gettime(CLOCK_MONOTONIC, &start);

            if (args.evolution == ORDERED) {
                evolve_ordered_serial(grid, width, height);
            } else if (args.evolution == STATIC) {
                evolve_static_serial(grid, next_grid, width, height);

                // pointer swapping
                uint8_t *temporary = grid;
                grid = next_grid;
                next_grid = temporary;
            } else if (args.evolution == WAVE) {
                // choose a new wave starting point for every generation
                start_row = rand() % height;
                start_column = rand() % width;

                evolve_wave_serial(grid, next_grid, width, height, start_row, start_column);
            } else if (args.evolution == WHITE_BLACK) {
                evolve_wb_serial(grid, next_grid, width, height);
            } else {
                fprintf(stderr, "evolution type not implemented yet\n");
                free(next_grid);
                free(grid);
                free(filename);
                free_arguments(&args);
                return 1;
            }

            if (args.fog_enabled)
                apply_fog_serial(grid, width, height, args.p_l);

            clock_gettime(CLOCK_MONOTONIC, &end);
            evolution_time += elapsed_time(start, end);

            if (args.dump_frequency > 0 && step % args.dump_frequency == 0) {
                clock_gettime(CLOCK_MONOTONIC, &start);

                if (write_snapshot(args.pattern_name, grid, width, height, step) != 0) {
                    free(next_grid);
                    free(grid);
                    free(filename);
                    free_arguments(&args);
                    return 1;
                }

                clock_gettime(CLOCK_MONOTONIC, &end);
                write_time += elapsed_time(start, end);
            }
        }

        if (args.dump_frequency == 0) {
            clock_gettime(CLOCK_MONOTONIC, &start);

            if (write_snapshot(args.pattern_name, grid, width, height, args.steps) != 0) {
                free(next_grid);
                free(grid);
                free(filename);
                free_arguments(&args);
                return 1;
            }

            clock_gettime(CLOCK_MONOTONIC, &end);
            write_time += elapsed_time(start, end);
        }

        if (args.benchmark) {
            const char *evolution_name;

            if (args.evolution == ORDERED)
                evolution_name = "ordered";
            else if (args.evolution == STATIC)
                evolution_name = "static";
            else if (args.evolution == WAVE)
                evolution_name = "wave";
            else
                evolution_name = "white-black";

            const char *repetition_string = getenv("BENCHMARK_REPETITION");
            int repetition = repetition_string != NULL ? (int)strtol(repetition_string, NULL, 10) : 0;
            double total_time = read_time + evolution_time + write_time;

            benchmark_write_result(evolution_name, width, height, args.steps,
                                    1, 1, repetition, read_time, evolution_time,
                                    write_time, total_time);
        } else {
            printf("read_time=%.6f evolution_time=%.6f write_time=%.6f\n",
                   read_time, evolution_time, write_time);
        }

        free(next_grid);
        free(grid);
        free(filename);
    }

    free_arguments(&args);
    return 0;
}

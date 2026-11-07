#include "evolution_wave.h"
#include "evolution_common.h"

#include <stdlib.h>

#define WAVE_OMP_MIN_ITEMS 256

static inline int wrap_coordinate(int coordinate, int size)
{
    if (coordinate < 0)
        return coordinate + size;
    if (coordinate >= size)
        return coordinate - size;
    return coordinate;
}

// number of distinct coordinates within periodic distance <= distance
static inline int axis_band_count(int distance, int size)
{
    if (distance >= size / 2)
        return size;
    return 2 * distance + 1;
}

// coordinate at position i within the periodic band
static inline int axis_band_coordinate(int start, int distance, int size, int i)
{
    if (distance >= size / 2)
        return i;
    return wrap_coordinate(start - distance + i, size);
}

// number of distinct coordinates at exactly the given periodic distance
static inline int axis_exact_count(int distance, int size)
{
    if (distance > size / 2)
        return 0;
    if (distance == 0 || (size % 2 == 0 && distance == size / 2))
        return 1;

    return 2;
}

// coordinate at position i among those at exactly the given distance
static inline int axis_exact_coordinate(int start, int distance, int size, int i)
{
    if (distance == 0)
        return start;
    if (axis_exact_count(distance, size) == 1)
        return wrap_coordinate(start + distance, size);

    return wrap_coordinate(start + (i == 0 ? -distance : distance), size);
}

// split a periodic band into at most two linear segments
static inline int axis_band_segments(int start, int distance, int size, int *begin, int *end)
{
    if (distance >= size / 2) {
        begin[0] = 0;
        end[0] = size;
        return 1;
    }

    int first = wrap_coordinate(start - distance, size);
    int length = 2 * distance + 1;

    if (first + length <= size) {
        begin[0] = first;
        end[0] = first + length;
        return 1;
    }

    begin[0] = first;
    end[0] = size;
    begin[1] = 0;
    end[1] = first + length - size;

    return 2;
}

static inline void update_cell_serial(uint8_t *grid, uint8_t *next_grid, int width, int height, int row, int column, int commit)
{
    int index = row * width + column;

    if (commit) {
        grid[index] = next_grid[index];
    } else {
        int live_neighbors = count_live_neighbors(grid, row, column, width, height);
        next_grid[index] = next_cell_state(grid[index], live_neighbors);
    }
}

static inline void update_cell_parallel(uint8_t *grid, uint8_t *next_grid, int width, int local_row, int column, int commit)
{
    int index = local_row * width + column;

    if (commit) {
        grid[index] = next_grid[index];
    } else {
        int live_neighbors = count_live_neighbors_parallel(grid, local_row, column, width);
        next_grid[index] = next_cell_state(grid[index], live_neighbors);
    }
}

// process one wavefront in the serial grid
static void wave_phase_serial(uint8_t *grid, uint8_t *next_grid, int width, int height, int start_row, int start_column, int distance, int commit)
{
    int extreme_rows = axis_exact_count(distance, height);
    int full_columns = axis_band_count(distance, width);

    // extreme rows: full column band
    for (int e = 0; e < extreme_rows; e++) {
        int row = axis_exact_coordinate(start_row, distance, height, e);

        for (int i = 0; i < full_columns; i++) {
            int column = axis_band_coordinate(start_column, distance, width, i);
            update_cell_serial(grid, next_grid, width, height, row, column, commit);
        }
    }

    // middle rows: only the extreme columns
    int side_columns = axis_exact_count(distance, width);

    if (distance > 0 && side_columns > 0) {
        int middle_rows = axis_band_count(distance - 1, height);

        for (int i = 0; i < middle_rows; i++) {
            int row = axis_band_coordinate(start_row, distance - 1, height, i);

            for (int s = 0; s < side_columns; s++) {
                int column = axis_exact_coordinate(start_column, distance, width, s);
                update_cell_serial(grid, next_grid, width, height, row, column, commit);
            }
        }
    }
}

// process the local part of one wavefront
static void wave_phase_parallel(uint8_t *grid, uint8_t *next_grid, int width, int height, int local_rows, int first_global_row, int start_row, int start_column, int distance, int commit)
{
    int full_columns = axis_band_count(distance, width);
    int side_columns = axis_exact_count(distance, width);
    int extreme_rows = axis_exact_count(distance, height);

    // extreme rows: full column band
    for (int e = 0; e < extreme_rows; e++) {
        int global_row = axis_exact_coordinate(start_row, distance, height, e);
        int local_row = global_row - first_global_row;

        if (local_row < 0 || local_row >= local_rows)
            continue;

#pragma omp parallel for schedule(static) if(full_columns > WAVE_OMP_MIN_ITEMS)
        for (int i = 0; i < full_columns; i++) {
            int column = axis_band_coordinate(start_column, distance, width, i);
            update_cell_parallel(grid, next_grid, width, local_row, column, commit);
        }
    }

    // middle rows: only the extreme columns
    if (distance > 0 && side_columns > 0) {
        int side0 = axis_exact_coordinate(start_column, distance, width, 0);
        int side1 = side_columns == 2 ? axis_exact_coordinate(start_column, distance, width, 1) : side0;

        int segment_begin[2], segment_end[2];
        int segments = axis_band_segments(start_row, distance - 1, height, segment_begin, segment_end);

        for (int g = 0; g < segments; g++) {
            int low = segment_begin[g] > first_global_row ? segment_begin[g] : first_global_row;
            int high_owned = first_global_row + local_rows;
            int high = segment_end[g] < high_owned ? segment_end[g] : high_owned;

#pragma omp parallel for schedule(static) if(high - low > WAVE_OMP_MIN_ITEMS)
            for (int global_row = low; global_row < high; global_row++) {
                int local_row = global_row - first_global_row;

                update_cell_parallel(grid, next_grid, width, local_row, side0, commit);

                if (side_columns == 2)
                    update_cell_parallel(grid, next_grid, width, local_row, side1, commit);
            }
        }
    }
}

void wave_seed(unsigned int seed)
{
    srand(seed);
}

void evolve_wave_serial(uint8_t *grid, uint8_t *next_grid, int width, int height, int start_row, int start_column)
{
    int max_distance = (width > height ? width : height) / 2;

    for (int distance = 0; distance <= max_distance; distance++) {
        // compute the current wavefront
        wave_phase_serial(grid, next_grid, width, height, start_row, start_column, distance, 0);

        // commit the current wavefront
        wave_phase_serial(grid, next_grid, width, height, start_row, start_column, distance, 1);
    }
}

void evolve_wave_parallel(uint8_t *grid, uint8_t *next_grid, int width, int height, int local_rows, int rank, int size, int start_row, int start_column, MPI_Comm comm)
{
    int base_rows = height / size;
    int remainder = height % size;
    int first_global_row = rank * base_rows + (rank < remainder ? rank : remainder);
    int max_distance = (width > height ? width : height) / 2;

    // exchange the initial grid boundary
    exchange_halos_parallel(grid, width, local_rows, rank, size, comm);

    for (int distance = 0; distance <= max_distance; distance++) {
        // compute the current local wavefront
        wave_phase_parallel(grid, next_grid, width, height, local_rows, first_global_row, start_row, start_column, distance, 0);

        MPI_Barrier(comm);

        // commit the current local wavefront
        wave_phase_parallel(grid, next_grid, width, height, local_rows, first_global_row, start_row, start_column, distance, 1);

        if (distance < max_distance) {
            // exchange updated boundary rows for the next wavefront
            exchange_halos_parallel(grid, width, local_rows, rank, size, comm);
            MPI_Barrier(comm);
        }
    }
}
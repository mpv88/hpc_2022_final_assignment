#include "grid.h"
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#define ALIVE_PROBABILITY 50

static uint32_t rng_next(uint32_t *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

int grid_initialize(uint8_t **data, int width, int height, unsigned int seed)
{
    size_t size = (size_t)width * (size_t)height;
    uint32_t state = seed;

    if (seed == 0)
        state = 1;

    *data = malloc(size);
    if (*data == NULL)
        return -1;

    for (size_t i = 0; i < size; i++)
        (*data)[i] = (rng_next(&state) % 100 < ALIVE_PROBABILITY) ? 1 : 0;

    return 0;
}

int grid_initialize_mpi(uint8_t **data, int width, int height, int *local_rows, int rank, int size, unsigned int seed)
{
    // mpi: determine the portion of rows assigned to this process
    int base_rows = height / size;
    int remainder = height % size;
    *local_rows = base_rows + (rank < remainder);

    size_t local_size = (size_t)(*local_rows) * (size_t)width;

    *data = malloc(local_size);
    if (*data == NULL)
        return -1;

    printf("MPI rank %d/%d: initializing %d rows\n", rank, size, *local_rows);

    // omp: initialize the local portion using multiple threads
#pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        uint32_t state = seed + (uint32_t)rank * 1000003u + (uint32_t)thread_id + 1u;

#pragma omp for
        for (size_t i = 0; i < local_size; i++)
            (*data)[i] = (rng_next(&state) % 100 < ALIVE_PROBABILITY) ? 1 : 0;
    }

    return 0;
}
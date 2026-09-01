#include "grid.h"
#include <stdlib.h>

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
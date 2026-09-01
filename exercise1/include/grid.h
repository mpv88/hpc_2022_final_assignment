/// \file grid.h
/// \author mpv
/// \brief header declaring functions to initialise the playground of GoL.

#ifndef GRID_H
#define GRID_H

#include <stdint.h>

/// \brief Initializes a playground serially with random live and dead cells.
/// \param data pointer to the grid data to allocate and initialize.
/// \param width playground width in pixels.
/// \param height playground height in pixels.
/// \param seed seed used to initialize the random number generator.
/// \return 0 on success, -1 if memory allocation fails.
int grid_initialize(uint8_t **data, int width, int height, unsigned int seed);


#endif
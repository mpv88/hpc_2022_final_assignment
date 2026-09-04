/// \file grid.h
/// \author mpv
/// \brief header declaring functions to initialise the playground of GoL.

#ifndef GRID_H
#define GRID_H

#include <stdint.h>

#define INITIALIZATION_SEED 1

/// \brief Initializes a playground serially with random live and dead cells.
/// \param data pointer to the grid data to allocate and initialize.
/// \param width playground width in pixels.
/// \param height playground height in pixels.
/// \param seed seed used to initialize the random number generator.
/// \return 0 on success, -1 if memory allocation fails.
int grid_initialize(uint8_t **data, int width, int height, unsigned int seed);


/// \brief Initializes the playground using MPI for row decomposition and OpenMP for local initialization.
/// \param data pointer to the local grid data to allocate and initialize.
/// \param width playground width in pixels.
/// \param height playground height in pixels.
/// \param local_rows pointer to the number of rows assigned to this MPI process.
/// \param rank MPI rank of the current process.
/// \param size total number of MPI processes.
/// \param seed seed used to initialize the random number generator.
/// \return 0 on success, -1 if memory allocation fails.
int grid_initialize_mpi(uint8_t **data, int width, int height, int *local_rows, int rank, int size, unsigned int seed);


#endif
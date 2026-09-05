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

/// \brief Allocates and initializes the portion of a grid assigned to an MPI rank.
/// \param data pointer receiving the address of the first real local row.
/// \param width width of the global grid in cells.
/// \param height height of the global grid in cells.
/// \param local_rows pointer receiving the number of real rows assigned to this rank.
/// \param rank MPI rank of the current process.
/// \param size total number of MPI processes.
/// \param seed initialization seed.
/// \return 0 on success, -1 if memory allocation fails.
/// NOTE:
/// Two additional ghost rows are allocated internally, one before and one after the real local rows.
/// The returned pointer refers to the first real row.
int grid_initialize_mpi(uint8_t **data, int width, int height, int *local_rows, int rank, int size, unsigned int seed);

#endif
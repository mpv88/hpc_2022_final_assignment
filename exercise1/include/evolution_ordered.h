/// \file evolution_ordered.h
/// \author mpv
/// \brief functions for the ordered GoL evolution pattern.

#ifndef EVOLUTION_ORDERED_H
#define EVOLUTION_ORDERED_H

#include <stdint.h>
#include <mpi.h>

/// \brief Performs one ordered evolution step on the complete grid.
/// \param grid pointer to the grid data.
/// \param width width of the grid in cells.
/// \param height height of the grid in cells.
void evolve_ordered_serial(uint8_t *grid, int width, int height);

/// \brief Performs one ordered evolution step on a grid distributed among MPI ranks.
/// \param grid pointer to the local grid data.
/// \param width width of the grid in cells.
/// \param local_rows number of rows owned by this MPI rank.
/// \param rank MPI rank of the current process.
/// \param size total number of MPI processes.
/// \param comm MPI communicator used for communication.
void evolve_ordered_parallel(uint8_t *grid, int width, int local_rows, int rank, int size, MPI_Comm comm);

#endif
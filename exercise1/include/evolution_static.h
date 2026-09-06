/// \file evolution_static.h
/// \author mpv
/// \brief functions for the static GoL evolution pattern.

#ifndef EVOLUTION_STATIC_H
#define EVOLUTION_STATIC_H

#include <stdint.h>
#include <mpi.h>

/// \brief Evolves a grid by one generation using synchronous static evolution.
/// \param current input grid representing the current generation.
/// \param next output grid receiving the next generation.
/// \param width width of the grid in cells.
/// \param height height of the grid in cells.
void evolve_static_serial(const uint8_t *current, uint8_t *next, int width, int height);

/// \brief Evolves the local portion of a grid by one generation using MPI and OpenMP.
/// \param current current local grid including its ghost rows.
/// \param next buffer receiving the next local generation.
/// \param width width of the global grid in cells.
/// \param local_rows number of real rows owned by this MPI rank.
/// \param rank MPI rank of the current process.
/// \param size total number of MPI processes.
/// \param comm MPI communicator used for communication.
void evolve_static_parallel(uint8_t *current, uint8_t *next, int width, int local_rows, int rank, int size, MPI_Comm comm);

#endif

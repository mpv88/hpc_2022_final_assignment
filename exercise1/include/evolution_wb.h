/// \file evolution_wb.h
/// \author mpv
/// \brief functions for the white-black GoL evolution pattern.

#ifndef EVOLUTION_WB_H
#define EVOLUTION_WB_H

#include <stdint.h>
#include <mpi.h>

/// \brief Evolves a grid by one generation using white-black ordering. 
/// \param grid grid to evolve.
/// \param next_grid auxiliary grid used between the two colour phases.
/// \param width width of the grid in cells.
/// \param height height of the grid in cells. 
void evolve_wb_serial(uint8_t *grid, uint8_t *next_grid, int width, int height);

/// \brief Evolves the local portion of a grid by one generation using MPI and OpenMP white-black ordering. 
/// \param grid local grid containing the current generation.
/// \param next_grid auxiliary local grid used between the two colour phases.
/// \param width width of the global grid in cells.
/// \param height height of the global grid in cells.
/// \param local_rows number of real rows owned by this MPI rank.
/// \param rank MPI rank of the current process.
/// \param size total number of MPI processes.
/// \param comm MPI communicator used for communication.
void evolve_wb_parallel(uint8_t *grid, uint8_t *next_grid, int width, int height, int local_rows, int rank, int size, MPI_Comm comm);

#endif
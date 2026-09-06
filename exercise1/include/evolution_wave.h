/// \file evolution_wave.h
/// \author mpv
/// \brief functions for the wave GoL evolution pattern.

#ifndef EVOLUTION_WAVE_H
#define EVOLUTION_WAVE_H

#include <stdint.h>
#include <mpi.h>

/// \brief Evolves a grid by one generation using synchronous wave evolution.
/// \param current input grid representing the current generation.
/// \param next output grid receiving the next generation.
/// \param width width of the grid in cells.
/// \param height height of the grid in cells.
/// \param start_row row of the wave starting cell.
/// \param start_column column of the wave starting cell.
void evolve_wave_serial(const uint8_t *current, uint8_t *next, int width, int height, int start_row, int start_column);

/// \brief Evolves the local portion of a grid by one generation using MPI and OpenMP wave evolution.
/// \param current current local grid including its ghost rows.
/// \param next buffer receiving the next local generation.
/// \param width width of the global grid in cells.
/// \param height height of the global grid in cells.
/// \param local_rows number of real rows owned by this MPI rank.
/// \param rank MPI rank of the current process.
/// \param size total number of MPI processes.
/// \param start_row global row of the wave starting cell.
/// \param start_column global column of the wave starting cell.
/// \param comm MPI communicator used for communication.
void evolve_wave_parallel(uint8_t *current, uint8_t *next, int width, int height, int local_rows, int rank, int size, int start_row, int start_column, MPI_Comm comm);

#endif


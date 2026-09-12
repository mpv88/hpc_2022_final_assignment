/// \file evolution_fog.h
/// \author mpv
/// \brief header declaring functions for the Finger of God (FoG) evolution mechanism in GoL.

#ifndef EVOLUTION_FOG_H
#define EVOLUTION_FOG_H

#include <stdint.h>
#include <mpi.h>

#define FOG_SEED 2

/// \brief Seeds the random generator used by the Finger of God mechanism.
/// \param seed seed used to initialize the random number generator.
void fog_seed(uint32_t seed);

/// \brief Applies one Finger of God event to the grid.
/// \param grid pointer to the grid data.
/// \param width playground width in pixels.
/// \param height playground height in pixels.
/// \param p_l probability parameter for the Finger of God event.
void apply_fog_serial(uint8_t *grid, int width, int height, double p_l);

/// \brief Applies one Finger of God event to a distributed grid.
/// \param grid pointer to the local portion of the grid (including ghost rows if present).
/// \param width width of the global grid in cells.
/// \param height height of the global grid in cells.
/// \param local_rows number of real rows owned by this rank.
/// \param start_row global index of the first real row owned by this rank.
/// \param rank MPI rank of the current process.
/// \param size total number of MPI processes.
/// \param p_l probability parameter for the Finger of God event.
/// \param comm MPI communicator used for coordination.
void apply_fog_parallel(uint8_t *grid, int width, int height, int local_rows, int start_row, int rank, int size, double p_l, MPI_Comm comm);

#endif






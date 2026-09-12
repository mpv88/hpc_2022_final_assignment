/// \file evolution_common.h
/// \author mpv
/// \brief common functions across all GoL evolution patterns.

#ifndef GOL_EVOLUTION_COMMON_H
#define GOL_EVOLUTION_COMMON_H

#include <mpi.h>
#include <stdint.h>

/// \brief Counts the live neighbors of a cell using periodic boundary conditions.
/// \param grid pointer to the grid data.
/// \param row row index of the cell.
/// \param column column index of the cell.
/// \param width width of the grid in cells.
/// \param height height of the grid in cells.
/// \return number of live neighbors of the cell.
int count_live_neighbors(const uint8_t *grid, int row, int column, int width, int height);

/// \brief Counts the live neighbors of a local cell using MPI ghost rows.
/// \param grid pointer to the first real local row.
/// \param row local row index of the cell, from 0 to local_rows - 1.
/// \param column column index of the cell.
/// \param width width of the grid in cells.
/// \return number of live neighbors of the cell.
int count_live_neighbors_parallel(const uint8_t *grid, int row, int column, int width);

/// \brief Determines the next state of a cell according to the Game of Life rules.
/// \param current_state current state of the cell.
/// \param live_neighbors number of live neighboring cells.
/// \return 1 if the cell is alive in the next state, 0 otherwise.
uint8_t next_cell_state(uint8_t current_state, int live_neighbors);

/// \brief Exchanges top and bottom halo rows of a distributed grid between neighboring MPI ranks.
/// \param grid pointer to the local grid portion including one halo row before and after the real rows.
/// \param width width of the grid in cells.
/// \param local_rows number of real rows owned by this rank (excluding halos).
/// \param rank MPI rank of the current process.
/// \param size total number of MPI processes.
/// \param comm MPI communicator used for the exchange.
void exchange_halos_parallel(uint8_t *grid, int width, int local_rows, int rank, int size, MPI_Comm comm);

#endif
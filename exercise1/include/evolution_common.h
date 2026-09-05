/// \file evolution_common.h
/// \author mpv
/// \brief common functions across all GoL evolution patterns.

#ifndef GOL_EVOLUTION_COMMON_H
#define GOL_EVOLUTION_COMMON_H

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

#endif
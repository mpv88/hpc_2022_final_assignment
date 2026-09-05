/// \file pgm.h
/// \author mpv
/// \brief header declaring functions to read and write raw 8-bit grayscale PGM files.

#ifndef PGM_H
#define PGM_H

#include <stdint.h>

/// \brief reads a PGM image from a file.
/// \param filename name of the PGM file to read.
/// \param data address of the pointer where the image data will be stored.
/// \param width address of the variable where the image width will be stored.
/// \param height address of the variable where the image height will be stored.
/// \return 0 on success, non-zero on error.
int pgm_read(const char *filename, uint8_t **data, int *width, int *height);

/// \brief  Writes a PGM image to a file.
/// \param filename name of the PGM file to create.
/// \param data  image data to write.
/// \param width image width in pixels.
/// \param height image height in pixels.
/// \return 0 on success, non-zero on error.
int pgm_write(const char *filename, const uint8_t *data, int width, int height);

/// \brief Reads a PGM image in parallel using MPI-IO and distributes its rows among MPI processes.
/// \param filename name of the PGM file to read.
/// \param data pointer to the buffer where the local image data will be stored.
/// \param width pointer to the image width in pixels.
/// \param local_rows pointer to the number of rows assigned to this MPI process.
/// \param height pointer to the total image height in pixels.
/// \param rank MPI rank of the current process.
/// \param size total number of MPI processes.
/// \return 0 on success, -1 if the file cannot be opened, -2 if the PGM header is invalid, -3 if memory allocation fails.
int pgm_read_mpi(const char *filename, uint8_t **data, int *width, int *local_rows, int *height, int rank, int size);

/// \brief Writes a PGM image in parallel using MPI-IO, with each MPI process writing its assigned rows.
/// \param filename name of the PGM file to write.
/// \param data pointer to the local image data to write.
/// \param width image width in pixels.
/// \param local_rows number of rows assigned to this MPI process.
/// \param height total image height in pixels.
/// \param rank MPI rank of the current process.
/// \param size total number of MPI processes.
/// \return 0 on success, -1 if the file cannot be opened, -2 if MPI file opening fails, -3 if writing fails.
int pgm_write_mpi(const char *filename, const uint8_t *data, int width, int local_rows, int height, int rank, int size);

#endif
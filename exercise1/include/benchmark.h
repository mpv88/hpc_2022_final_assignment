/// \file benchmark.h
/// \author mpv
/// \brief Benchmarking utilities for timing and csv output of Game of Life simulations.

#ifndef BENCHMARK_H
#define BENCHMARK_H

/// \brief Writes the csv header used by benchmark output.
void benchmark_write_header(void);

/// \brief Writes one benchmark result as a csv row.
/// \param evolution name or identifier of the evolution algorithm.
/// \param width width of the grid in cells.
/// \param height height of the grid in cells.
/// \param steps number of simulation steps executed.
/// \param mpi_tasks number of MPI tasks used.
/// \param omp_threads number of OpenMP threads used.
/// \param repetition repetition index of this benchmark run.
/// \param read_time time spent reading input data, in seconds.
/// \param evolution_time time spent running the evolution, in seconds.
/// \param write_time time spent writing output data, in seconds.
/// \param total_time total elapsed time for the benchmark, in seconds.
void benchmark_write_result(const char *evolution, int width, int height, int steps,
                            int mpi_tasks, int omp_threads, int repetition,
                            double read_time, double evolution_time,
                            double write_time, double total_time);

#endif
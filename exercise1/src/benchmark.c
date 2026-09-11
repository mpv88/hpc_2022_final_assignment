#include "benchmark.h"

#include <stdio.h>

void benchmark_write_header(void)
{
    printf("evolution,grid_width,grid_height,steps,mpi_tasks,omp_threads,repetition,read_time,evolution_time,write_time,total_time\n");
}

void benchmark_write_result(const char *evolution, int width, int height, int steps, int mpi_tasks, int omp_threads, int repetition, double read_time, double evolution_time, double write_time, double total_time)
{
    printf("%s,%d,%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f\n",
           evolution,
           width,
           height,
           steps,
           mpi_tasks,
           omp_threads,
           repetition,
           read_time,
           evolution_time,
           write_time,
           total_time);
}
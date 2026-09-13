# Exercise 1 — Conway's Game of Life

Implementation of Conway’s Game of Life in C, including a serial version and a hybrid MPI + OpenMP version. Four evolution strategies are available: Ordered, Static, Wave and White–Black. The program supports PGM input/output, optional Finger of God (FoG) perturbations, snapshots and a benchmark mode. The playground is a 2D grid where each side must be at least 100 cells, with no upper bound; square grids are included as a special case.

## Requirements

The following software is required:

* C11-compatible compiler
* MPI
* OpenMP
* CMake
* Make

## Project structure

```text
exercise1/
├── CMakeLists.txt
├── Makefile
├── README.md
├── Report.pdf
├── doxygen/        # documentation
├── figs/           # plots for the experiments
├── include/        # header files
├── patterns/       # input, output and reference patterns
├── results/        # benchmark results (.csv)
├── scripts/        # Slurm batch scripts
├── src/            # source files
└── tests/          # unit tests
```

The main program is organized as follows:

```text
Serial
└── main_serial.c
    └── evolution

MPI + OpenMP
└── main_parallel.c
    ├── MPI process
    │   └── main thread
    │       ├── MPI communication
    │       └── OpenMP computation
    │           ├── thread 0
    │           ├── thread 1
    │           ├── ...
    │           └── thread N
    └── ...
```

## Compilation

Using Make:

```bash
make
```

Using CMake:

```bash
cmake -S . -B build
cmake --build build
```

MPI must be available when compiling the parallel version.

The unit tests can be built separately if necessary.

## Usage

### Arguments

| Argument          | Meaning                                     |
| ----------------- | ------------------------------------------- |
| `-i`              | initialize a playground                     |
| `-r`              | run a playground                            |
| `-k <value>`      | size of the square playground               |
| `-w <value>`      | width of the playground                     |
| `-h <value>`      | height of the playground                    |
| `-e [0\|1\|2\|3]` | evolution type                              |
| `-f <string>`     | input/output file name                      |
| `-n <value>`      | number of steps                             |
| `-s <value>`      | snapshot frequency (`0` = final state only) |
| `-g <p_L>`        | enable Finger of God with probability `p_L` |
| `-b`              | enable benchmark mode                       |

If `-w` and `-h` are not specified, `-k` defines a square playground.

Evolution types:

```text
0 = ordered
1 = static
2 = wave
3 = white-black
```

For FoG:

```text
p_L = probability of setting the randomly selected cell alive
p_D = 1 - p_L
```

## Examples

Initialize a rectangular 80×120 playground:

```bash
./build/gol_serial -i -w 80 -h 120 -f my_pattern
```

Run the serial version for 100 steps using Static evolution and save a snapshot only for last step:

```bash
./build/gol_serial -r -f my_pattern -e 1 -n 100 -s 0
```

Run the MPI + OpenMP version for 100 steps using Wave evolution with 4 MPI processes:

```bash
mpirun -np 4 ./build/gol -r -f my_pattern -e 2 -n 100
```

Run the hybrid version using 2 MPI processes and 4 OpenMP threads per process with White-Black evolution:

```bash
OMP_NUM_THREADS=4 mpirun -np 2 ./build/gol -r -f my_pattern -e 3 -n 100
```

Run the hybrid version using Ordered evolution with Finger of God enabled and `p_L = 0.05`:

```bash
OMP_NUM_THREADS=4 mpirun -np 2 ./build/gol -r -f my_pattern -e 0 -n 100 -g 0.05
```

Save a snapshot after every step using Static evolution:

```bash
mpirun -np 4 ./build/gol -r -f my_pattern -e 1 -n 100 -s 1
```

## Output

The program reads and writes playgrounds using the PGM format.

* `-s 0` saves only the final state.
* `-s N` saves a snapshot every `N` steps.
* The output files are written using the specified pattern name.

In benchmark mode (`-b`), the program writes one CSV result containing the measured execution times and run configuration.

## Benchmark mode

Benchmark mode can be enabled with:

```bash
-b
```

For each run, the program separately measures:

* PGM read time
* evolution time
* PGM write time
* total execution time

The benchmark also records the actual number of MPI processes and OpenMP threads used. The environment variable `BENCHMARK_REPETITION` identifies repeated benchmark runs.

## Documentation

Doxygen documentation can be generated with:

```bash
doxygen Doxyfile
```

The generated documentation is placed in the `doxygen/` directory.
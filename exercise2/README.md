# Exercise 2 — GEMM Benchmark

Performance comparison of the BLAS level-3 GEMM operation using three numerical libraries: Intel Math Kernel Library (MKL), OpenBLAS and BLIS. Both single-precision and double-precision arithmetic are evaluated on different HPC architectures.

## Requirements
The following software is required:
* C compiler
* Make
* BLAS library: MKL, OpenBLAS and BLIS
* Slurm for HPC experiments

## Project structure
```text
exercise2/
├── Makefile    
├── Makefile.local  # local Make (test)
├── README.md
├── figs/           # generated plots
├── results/        # benchmark results (.csv)
├── scripts/        # local and Slurm benchmark scripts
└── gemm.c          # GEMM benchmark source
```

## Compilation
For local compilation, use the local Makefile:
```bash
make -f Makefile.local
```

For HPC compilation on ORFEO, the target architecture must be specified:
```bash
make ARCH=epyc
make ARCH=thin
```

The architecture argument selects the corresponding BLIS installation:
```text
EPYC → ~/myblis/epyc
THIN → ~/myblis/thin
```
The HPC Makefile provides separate executables for each library and precision:
```text
build/
├── gemm_mkl_float.x
├── gemm_mkl_double.x
├── gemm_openblas_float.x
├── gemm_openblas_double.x
├── gemm_blis_float.x
└── gemm_blis_double.x
```
The executables are compiled with `-O3` and `-march=native`.

## Usage
Each executable accepts the GEMM dimensions in the order `M K N`:
```bash
./build/gemm_mkl_double.x M K N
```

These dimensions define the matrices as:
```text
A: M × K
B: K × N
C: M × N
```

For example:
```bash
./build/gemm_mkl_double.x 2000 200 1000
```
corresponds to:
```text
A: 2000 × 200
B: 200 × 1000
C: 2000 × 1000
```

The benchmark computes the matrix product:
```text
C = A × B
```

The output of each run is a `.csv` file containing the matrix dimensions, execution time and achieved performance in GFLOPS:
```text
2000,2000,2000,0.123456,129.54
```

### HPC batch jobs
The scalability experiments are executed through the Slurm batch scripts in `scripts/`. The scripts take the target architecture and thread affinity as arguments. The core-scalability script also takes the matrix size.

For matrix-size scalability, the scripts are launched as:
```bash
scripts/size_scalability.sh EPYC spread
scripts/size_scalability.sh EPYC close
scripts/size_scalability.sh THIN spread
scripts/size_scalability.sh THIN close
```
For core-count scalability, the scripts are launched as:
```bash
scripts/core_scalability.sh EPYC 10000 spread
scripts/core_scalability.sh EPYC 10000 close
scripts/core_scalability.sh EPYC 20000 spread
scripts/core_scalability.sh EPYC 20000 close

scripts/core_scalability.sh THIN 10000 spread
scripts/core_scalability.sh THIN 10000 close
scripts/core_scalability.sh THIN 20000 spread
scripts/core_scalability.sh THIN 20000 close
```
On ORFEO, the jobs are submitted on one node with enough CPUs for the largest core count tested:
```bash
sbatch --partition=EPYC --cpus-per-task=64 --mem=490G scripts/size_scalability.sh EPYC spread
sbatch --partition=EPYC --cpus-per-task=64 --mem=490G scripts/size_scalability.sh EPYC close

sbatch --partition=THIN --nodelist=thin[002-003,007-008] --cpus-per-task=12 --mem=740G scripts/size_scalability.sh THIN spread
sbatch --partition=THIN --nodelist=thin[002-003,007-008] --cpus-per-task=12 --mem=740G scripts/size_scalability.sh THIN close

sbatch --partition=EPYC --cpus-per-task=64 --mem=490G scripts/core_scalability.sh EPYC 10000 spread
sbatch --partition=EPYC --cpus-per-task=64 --mem=490G scripts/core_scalability.sh EPYC 10000 close
sbatch --partition=EPYC --cpus-per-task=64 --mem=490G scripts/core_scalability.sh EPYC 20000 spread
sbatch --partition=EPYC --cpus-per-task=64 --mem=490G scripts/core_scalability.sh EPYC 20000 close

sbatch --partition=THIN --nodelist=thin[002-003,007-008] --cpus-per-task=12 --mem=740G scripts/core_scalability.sh THIN 10000 spread
sbatch --partition=THIN --nodelist=thin[002-003,007-008] --cpus-per-task=12 --mem=740G scripts/core_scalability.sh THIN 10000 close
sbatch --partition=THIN --nodelist=thin[002-003,007-008] --cpus-per-task=12 --mem=740G scripts/core_scalability.sh THIN 20000 spread
sbatch --partition=THIN --nodelist=thin[002-003,007-008] --cpus-per-task=12 --mem=740G scripts/core_scalability.sh THIN 20000 close
```
The scripts set `OMP_NUM_THREADS` according to the number of cores used for each benchmark run and set `OMP_PROC_BIND` to the requested `spread` or `close` affinity. Each executable is launched with `srun`.

Note that the THIN partition contains both 36-core `fat` nodes and 24-core `thin` nodes. The commands therefore explicitly select the `thin` nodes to ensure that the experiments use the intended 12-core-per-socket THIN architecture.

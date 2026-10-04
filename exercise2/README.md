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

The scalability experiments are executed through the Slurm batch scripts in `scripts/`. Each job runs one library and one precision. The scripts take the architecture, precision, library and thread affinity as arguments. The core-scalability script also takes the matrix size.

For matrix-size scalability:

```bash
scripts/size_scalability.sh EPYC float mkl spread
scripts/size_scalability.sh EPYC float mkl close
scripts/size_scalability.sh THIN double openblas spread
scripts/size_scalability.sh THIN double openblas close
```

The full matrix-size experiment requires all combinations of:

* architectures: `EPYC`, `THIN`
* precisions: `float`, `double`
* libraries: `mkl`, `openblas`, `blis`
* affinities: `spread`, `close`

For core-count scalability:

```bash
scripts/core_scalability.sh EPYC 10000 float mkl spread
scripts/core_scalability.sh EPYC 10000 float mkl close
scripts/core_scalability.sh THIN 20000 double openblas spread
scripts/core_scalability.sh THIN 20000 double openblas close
```

The full core-count experiment requires all combinations of:

* architectures: `EPYC`, `THIN`
* matrix sizes: `10000`, `20000`
* precisions: `float`, `double`
* libraries: `mkl`, `openblas`, `blis`
* affinities: `spread`, `close`

Both scripts additionally support optional NUMA memory policies:

```bash
scripts/size_scalability.sh THIN float mkl spread membind 0,1 --balancing
scripts/size_scalability.sh THIN float mkl spread interleave 0,1,2,3
scripts/core_scalability.sh EPYC 10000 float mkl spread membind 0,1
```
The `default` policy is used when no NUMA arguments are provided. `membind` and `interleave` require NUMA nodes, while `--balancing` is an optional runtime flag for `membind`.


On ORFEO, the jobs are submitted on one node with enough CPUs for the largest core count tested. Example submissions are:
```bash
sbatch --partition=EPYC --cpus-per-task=64 --mem=490G scripts/size_scalability.sh EPYC float mkl spread
sbatch --partition=EPYC --cpus-per-task=64 --mem=490G scripts/size_scalability.sh EPYC float mkl close

sbatch --partition=THIN --nodelist=thin003 --cpus-per-task=12 --mem=740G scripts/size_scalability.sh THIN double openblas spread
sbatch --partition=THIN --nodelist=thin007 --cpus-per-task=12 --mem=740G scripts/size_scalability.sh THIN double openblas close

sbatch --partition=EPYC --cpus-per-task=64 --mem=490G scripts/core_scalability.sh EPYC 10000 float mkl spread
sbatch --partition=EPYC --cpus-per-task=64 --mem=490G scripts/core_scalability.sh EPYC 20000 double openblas close

sbatch --partition=THIN --nodelist=thin003 --cpus-per-task=12 --mem=740G scripts/core_scalability.sh THIN 10000 float mkl spread
sbatch --partition=THIN --nodelist=thin008 --cpus-per-task=12 --mem=740G scripts/core_scalability.sh THIN 20000 double openblas close
```

For the additional THIN NUMA experiments, 24 CPUs are requested so that both sockets are used. For example:
```bash
sbatch --partition=THIN --nodelist=thin003 --cpus-per-task=24 --mem=740G scripts/size_scalability.sh THIN float mkl spread membind 0,1 --balancing
sbatch --partition=THIN --nodelist=thin003 --cpus-per-task=24 --mem=740G scripts/size_scalability.sh THIN float mkl spread interleave 0,1,2,3
```
The jobs are submitted on one node with enough CPUs for the largest core count tested. The ORFEO THIN partition contains both 36-core `fat` nodes and 24-core `thin` nodes; the commands explicitly select the `thin` nodes when the 24-core configuration is required.
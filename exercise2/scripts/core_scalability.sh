#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name=gemm_core
#SBATCH --get-user-env
#SBATCH --chdir=/u/dssc/mpivid00/hpc_2022_final_assignment/exercise2
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output=gemm_core_%j.out

# architecture, size, precision, library and affinity
ARCHITECTURE=$1
SIZE=$2
PRECISION=$3
LIBRARY=$4
AFFINITY=$5

if [[ "$ARCHITECTURE" == "EPYC" ]]; then
    MAX_CORES=64
elif [[ "$ARCHITECTURE" == "THIN" ]]; then
    MAX_CORES=12
else
    echo "usage: sbatch --partition=PARTITION --cpus-per-task=CORES core_scalability.sh EPYC|THIN SIZE float|double mkl|openblas|blis spread|close"
    exit 1
fi

if [[ "$PRECISION" != "float" && "$PRECISION" != "double" ]]; then
    echo "usage: sbatch --partition=PARTITION --cpus-per-task=CORES core_scalability.sh EPYC|THIN SIZE float|double mkl|openblas|blis spread|close"
    exit 1
fi

if [[ "$LIBRARY" != "mkl" && "$LIBRARY" != "openblas" && "$LIBRARY" != "blis" ]]; then
    echo "usage: sbatch --partition=PARTITION --cpus-per-task=CORES core_scalability.sh EPYC|THIN SIZE float|double mkl|openblas|blis spread|close"
    exit 1
fi

if [[ "$AFFINITY" != "spread" && "$AFFINITY" != "close" ]]; then
    echo "usage: sbatch --partition=PARTITION --cpus-per-task=CORES core_scalability.sh EPYC|THIN SIZE float|double mkl|openblas|blis spread|close"
    exit 1
fi

# build directory
BUILD_DIR="build"

# modules
module load tbb
module load compiler-rt
module load mkl/2025.3
module load openBLAS/0.3.29-omp

# thread config
export OMP_PLACES=cores
export OMP_PROC_BIND="$AFFINITY"

# BLIS library path
export LD_LIBRARY_PATH="$HOME/myblis/${ARCHITECTURE,,}/lib:$LD_LIBRARY_PATH"

# set parameters
REPETITIONS=10

# output
NOW=$(date +"%Y-%m-%d_%H-%M-%S")
HOST=$(hostname)
OUTPUT_DIR="results"
CSV_FILE="$OUTPUT_DIR/gemm_core_${ARCHITECTURE}_${SIZE}_${PRECISION}_${LIBRARY}_${AFFINITY}_${NOW}.csv"

mkdir -p "$OUTPUT_DIR"

echo "library,precision,architecture,node,m,k,n,cores,threads,affinity,repetition,time_s,gflops" > "$CSV_FILE"

# run experiment
echo "start core scalability"
echo "architecture: $ARCHITECTURE"
echo "precision: $PRECISION"
echo "library: $LIBRARY"
echo "host: $HOST"
echo "matrix size: $SIZE"
echo "maximum cores: $MAX_CORES"
echo "affinity: $AFFINITY"
echo "repetitions: $REPETITIONS"

for REPETITION in $(seq 1 "$REPETITIONS"); do
    for CORES in $(seq 1 "$MAX_CORES"); do
        export OMP_NUM_THREADS="$CORES"
        export BLIS_NUM_THREADS="$CORES"
        EXECUTABLE="$BUILD_DIR/gemm_${LIBRARY}_${PRECISION}.x"
        RESULT=$(srun --exclusive -n1 --cpus-per-task="$CORES" "$EXECUTABLE" "$SIZE" "$SIZE" "$SIZE")
        IFS=',' read -r M K N TIME GFLOPS <<< "$RESULT"
        echo "$LIBRARY,$PRECISION,$ARCHITECTURE,$HOST,$M,$K,$N,$CORES,$OMP_NUM_THREADS,$OMP_PROC_BIND,$REPETITION,$TIME,$GFLOPS" >> "$CSV_FILE"
    done
done

echo "completed"
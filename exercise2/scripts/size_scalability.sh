#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name=gemm_size
#SBATCH --get-user-env
#SBATCH --chdir=/u/dssc/mpivid00/assignment/exercise2
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output=gemm_size_%j.out

# architecture
ARCHITECTURE=$1

if [[ "$ARCHITECTURE" == "EPYC" ]]; then
    CORES=64
elif [[ "$ARCHITECTURE" == "THIN" ]]; then
    CORES=12
else
    echo "usage: sbatch --partition=PARTITION --cpus-per-task=CORES size_scalability.sh EPYC|THIN"
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
export OMP_PROC_BIND=spread
export OMP_NUM_THREADS=$CORES
export BLIS_NUM_THREADS=$CORES

# BLIS library path
export LD_LIBRARY_PATH="$HOME/myblis/${ARCHITECTURE,,}/lib:$LD_LIBRARY_PATH"

# set parameters
REPETITIONS=10
SIZE_START=2000
SIZE_END=20000
SIZE_STEP=500

# libs and corresponding executables
LIBRARIES=("mkl" "openblas" "blis")
PRECISIONS=("float" "double")

# output
NOW=$(date +"%Y-%m-%d_%H-%M-%S")
HOST=$(hostname)
OUTPUT_DIR="results"
CSV_FILE="$OUTPUT_DIR/gemm_size_${ARCHITECTURE}_${NOW}.csv"

mkdir -p "$OUTPUT_DIR"

echo "library,precision,architecture,node,m,k,n,cores,threads,affinity,repetition,time_s,gflops" > "$CSV_FILE"

# run experiment
echo "start matrix-size scalability"
echo "architecture: $ARCHITECTURE"
echo "host: $HOST"
echo "cores: $CORES"
echo "repetitions: $REPETITIONS"
echo "matrix sizes: $SIZE_START-$SIZE_END"

for PRECISION in "${PRECISIONS[@]}"; do
    for LIBRARY in "${LIBRARIES[@]}"; do
        for REPETITION in $(seq 1 "$REPETITIONS"); do
            for SIZE in $(seq "$SIZE_START" "$SIZE_STEP" "$SIZE_END"); do
                EXECUTABLE="$BUILD_DIR/gemm_${LIBRARY}_${PRECISION}.x"
                RESULT=$(srun --exclusive -n1 --cpus-per-task="$CORES" "$EXECUTABLE" "$SIZE" "$SIZE" "$SIZE")
                IFS=',' read -r M K N TIME GFLOPS <<< "$RESULT"
                echo "$LIBRARY,$PRECISION,$ARCHITECTURE,$HOST,$M,$K,$N,$CORES,$OMP_NUM_THREADS,$OMP_PROC_BIND,$REPETITION,$TIME,$GFLOPS" >> "$CSV_FILE"
            done
        done
    done
done

echo "completed"
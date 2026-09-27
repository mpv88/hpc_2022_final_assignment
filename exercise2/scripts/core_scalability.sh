#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name=gemm_core
#SBATCH --get-user-env
#SBATCH --chdir=TODO: add my path
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output=gemm_core_%j.out

# architecture
ARCHITECTURE=$1

if [[ "$ARCHITECTURE" == "EPYC" ]]; then
    MAX_CORES=64
elif [[ "$ARCHITECTURE" == "THIN" ]]; then
    MAX_CORES=12
else
    echo "usage: sbatch --partition=PARTITION --ntasks-per-node=CORES core_scalability.sh EPYC|THIN"
    exit 1
fi

# build directory
BUILD_DIR="build"

# modules
module load mkl
module load openBLAS/0.3.23-omp

# thread config
export OMP_PLACES=cores
export OMP_PROC_BIND=spread

# BLIS library path
export LD_LIBRARY_PATH=TODO: add my path

# set parameters
SIZE=TODO: choose intermediate matrix size
REPETITIONS=10

# libs and corresponding executables
LIBRARIES=("mkl" "openblas" "blis")
PRECISIONS=("float" "double")

# output
NOW=$(date +"%Y-%m-%d_%H-%M-%S")
HOST=$(hostname)
OUTPUT_DIR="results"
CSV_FILE="$OUTPUT_DIR/gemm_core_${ARCHITECTURE}_${NOW}.csv"

mkdir -p "$OUTPUT_DIR"

echo "library,precision,node,m,k,n,cores,threads,affinity,repetition,time_s,gflops" > "$CSV_FILE"

# run experiment
echo "start core scalability"
echo "architecture: $ARCHITECTURE"
echo "host: $HOST"
echo "matrix size: $SIZE"
echo "maximum cores: $MAX_CORES"
echo "repetitions: $REPETITIONS"

for PRECISION in "${PRECISIONS[@]}"; do
    for LIBRARY in "${LIBRARIES[@]}"; do
        for REPETITION in $(seq 1 "$REPETITIONS"); do
            for CORES in $(seq 1 "$MAX_CORES"); do
                export OMP_NUM_THREADS="$CORES"
                export BLIS_NUM_THREADS="$CORES"
                EXECUTABLE="$BUILD_DIR/gemm_${LIBRARY}_${PRECISION}.x"
                RESULT=$(srun --exclusive -n1 --cpus-per-task="$CORES" "$EXECUTABLE" "$SIZE" "$SIZE" "$SIZE")
                IFS=',' read -r M K N TIME GFLOPS <<< "$RESULT"
                echo "$LIBRARY,$PRECISION,$ARCHITECTURE,$M,$K,$N,$CORES,$OMP_NUM_THREADS,$OMP_PROC_BIND,$REPETITION,$TIME,$GFLOPS" >> "$CSV_FILE"
            done
        done
    done
done

echo "completed"
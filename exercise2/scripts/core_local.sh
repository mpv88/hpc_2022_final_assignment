#!/bin/bash

# architecture
ARCHITECTURE="LOCAL"

# build directory
BUILD_DIR="build"

# available CPU cores
MAX_CORES=$(nproc)

# thread config
export OMP_PLACES=cores
export OMP_PROC_BIND=spread

# set parameters
SIZES=(500 1000)
REPETITIONS=10

# core counts to test
CORES_LIST=(1 2 4 8 16 32)

# libs and corresponding executables
LIBRARIES=("mkl" "openblas" "blis")
PRECISIONS=("float" "double")

# output
NOW=$(date +"%Y-%m-%d_%H-%M-%S")
HOST=$(hostname)
OUTPUT_DIR="results"
CSV_FILE="$OUTPUT_DIR/gemm_core_${ARCHITECTURE}_${NOW}.csv"

mkdir -p "$OUTPUT_DIR"

echo "library,precision,architecture,node,m,k,n,cores,threads,affinity,repetition,time_s,gflops" > "$CSV_FILE"

# run experiment
echo "start local core scalability"
echo "host: $HOST"
echo "available cores: $MAX_CORES"
echo "tested cores: ${CORES_LIST[*]}"
echo "matrix sizes: ${SIZES[*]}"
echo "repetitions: $REPETITIONS"

for SIZE in "${SIZES[@]}"; do
    for PRECISION in "${PRECISIONS[@]}"; do
        for LIBRARY in "${LIBRARIES[@]}"; do
            for REPETITION in $(seq 1 "$REPETITIONS"); do
                for CORES in "${CORES_LIST[@]}"; do
                    export OMP_NUM_THREADS="$CORES"
                    export BLIS_NUM_THREADS="$CORES"
                    EXECUTABLE="$BUILD_DIR/gemm_${LIBRARY}_${PRECISION}.x"
                    RESULT=$("$EXECUTABLE" "$SIZE" "$SIZE" "$SIZE")
                    IFS=',' read -r M K N TIME GFLOPS <<< "$RESULT"
                    echo "$LIBRARY,$PRECISION,$ARCHITECTURE,$HOST,$M,$K,$N,$CORES,$OMP_NUM_THREADS,$OMP_PROC_BIND,$REPETITION,$TIME,$GFLOPS" >> "$CSV_FILE"
                done
            done
        done
    done
done

echo "completed"
echo "output: $CSV_FILE"
#!/bin/bash

# architecture
ARCHITECTURE="LOCAL"

# build directory
BUILD_DIR="build"

# available CPU cores
CORES=$(nproc)

# thread config
export OMP_PLACES=cores
export OMP_PROC_BIND=spread
export OMP_NUM_THREADS=$CORES
export BLIS_NUM_THREADS=$CORES

# set parameters
REPETITIONS=10
SIZES=(500 1000)

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
echo "start local matrix-size scalability"
echo "architecture: $ARCHITECTURE"
echo "host: $HOST"
echo "cores: $CORES"
echo "repetitions: $REPETITIONS"
echo "matrix sizes: ${SIZES[*]}"

for PRECISION in "${PRECISIONS[@]}"; do
    for LIBRARY in "${LIBRARIES[@]}"; do
        for REPETITION in $(seq 1 "$REPETITIONS"); do
            for SIZE in "${SIZES[@]}"; do
                EXECUTABLE="$BUILD_DIR/gemm_${LIBRARY}_${PRECISION}.x"
                RESULT=$("$EXECUTABLE" "$SIZE" "$SIZE" "$SIZE")
                IFS=',' read -r M K N TIME GFLOPS <<< "$RESULT"
                echo "$LIBRARY,$PRECISION,$ARCHITECTURE,$HOST,$M,$K,$N,$CORES,$OMP_NUM_THREADS,$OMP_PROC_BIND,$REPETITION,$TIME,$GFLOPS" >> "$CSV_FILE"
            done
        done
    done
done

echo "completed"
echo "output: $CSV_FILE"
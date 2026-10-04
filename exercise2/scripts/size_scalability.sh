#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name=gemm_size
#SBATCH --get-user-env
#SBATCH --chdir=/u/dssc/mpivid00/hpc_2022_final_assignment/exercise2
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output=gemm_size_%j.out

# architecture, precision, library, affinity and optional NUMA policy
ARCHITECTURE=$1
PRECISION=$2
LIBRARY=$3
AFFINITY=$4
MEMORY_POLICY=${5:-default}
NUMA_NODES=${6:-}
BALANCING=${7:-}

USAGE="usage: sbatch --partition=PARTITION --cpus-per-task=CORES size_scalability.sh EPYC|THIN float|double mkl|openblas|blis spread|close [default|interleave|membind] [NUMA_NODES] [--balancing]"

if [[ "$ARCHITECTURE" == "EPYC" ]]; then
    CORES=64
elif [[ "$ARCHITECTURE" == "THIN" ]]; then
    CORES=24
else
    echo "$USAGE"
    exit 1
fi

if [[ "$PRECISION" != "float" && "$PRECISION" != "double" ]]; then
    echo "$USAGE"
    exit 1
fi

if [[ "$LIBRARY" != "mkl" && "$LIBRARY" != "openblas" && "$LIBRARY" != "blis" ]]; then
    echo "$USAGE"
    exit 1
fi

if [[ "$AFFINITY" != "spread" && "$AFFINITY" != "close" ]]; then
    echo "$USAGE"
    exit 1
fi

if [[ "$MEMORY_POLICY" != "default" && "$MEMORY_POLICY" != "interleave" && "$MEMORY_POLICY" != "membind" ]]; then
    echo "$USAGE"
    exit 1
fi

if [[ "$MEMORY_POLICY" == "default" && -n "$NUMA_NODES" ]]; then
    echo "NUMA nodes cannot be specified with the default memory policy"
    exit 1
fi

if [[ "$MEMORY_POLICY" != "default" && -z "$NUMA_NODES" ]]; then
    echo "NUMA nodes must be specified for $MEMORY_POLICY"
    exit 1
fi

if [[ -n "$BALANCING" && "$BALANCING" != "--balancing" ]]; then
    echo "$USAGE"
    exit 1
fi

if [[ "$BALANCING" == "--balancing" && "$MEMORY_POLICY" != "membind" ]]; then
    echo "--balancing can only be used with membind"
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
export OMP_NUM_THREADS="$CORES"
export BLIS_NUM_THREADS="$CORES"

# BLIS library path
export LD_LIBRARY_PATH="$HOME/myblis/${ARCHITECTURE,,}/lib:$LD_LIBRARY_PATH"

# set parameters
REPETITIONS=10
SIZE_START=2000
SIZE_END=20000
SIZE_STEP=500

# output
NOW=$(date +"%Y-%m-%d_%H-%M-%S")
HOST=$(hostname)
OUTPUT_DIR="results"
CSV_FILE="$OUTPUT_DIR/gemm_size_${ARCHITECTURE}_${PRECISION}_${LIBRARY}_${AFFINITY}_${NOW}.csv"

mkdir -p "$OUTPUT_DIR"

echo "library,precision,architecture,node,m,k,n,cores,threads,affinity,memory_policy,numa_nodes,repetition,time_s,gflops" > "$CSV_FILE"

# run experiment
echo "start matrix-size scalability"
echo "architecture: $ARCHITECTURE"
echo "precision: $PRECISION"
echo "library: $LIBRARY"
echo "host: $HOST"
echo "cores: $CORES"
echo "affinity: $AFFINITY"
echo "memory policy: $MEMORY_POLICY"
echo "NUMA nodes: ${NUMA_NODES:-none}"
echo "balancing: ${BALANCING:-disabled}"
echo "repetitions: $REPETITIONS"
echo "matrix sizes: $SIZE_START-$SIZE_END"

for REPETITION in $(seq 1 "$REPETITIONS"); do
    for SIZE in $(seq "$SIZE_START" "$SIZE_STEP" "$SIZE_END"); do
        EXECUTABLE="$BUILD_DIR/gemm_${LIBRARY}_${PRECISION}.x"

        if [[ "$MEMORY_POLICY" == "default" ]]; then
            RESULT=$(srun --exclusive -n1 --cpus-per-task="$CORES" "$EXECUTABLE" "$SIZE" "$SIZE" "$SIZE")
        elif [[ "$MEMORY_POLICY" == "interleave" ]]; then
            RESULT=$(srun --exclusive -n1 --cpus-per-task="$CORES" numactl --interleave="$NUMA_NODES" "$EXECUTABLE" "$SIZE" "$SIZE" "$SIZE")
        elif [[ "$MEMORY_POLICY" == "membind" ]]; then
            if [[ "$BALANCING" == "--balancing" ]]; then
                RESULT=$(srun --exclusive -n1 --cpus-per-task="$CORES" numactl --membind="$NUMA_NODES" --balancing "$EXECUTABLE" "$SIZE" "$SIZE" "$SIZE")
            else
                RESULT=$(srun --exclusive -n1 --cpus-per-task="$CORES" numactl --membind="$NUMA_NODES" "$EXECUTABLE" "$SIZE" "$SIZE" "$SIZE")
            fi
        fi

        IFS=',' read -r M K N TIME GFLOPS <<< "$RESULT"
        echo "$LIBRARY,$PRECISION,$ARCHITECTURE,$HOST,$M,$K,$N,$CORES,$OMP_NUM_THREADS,$OMP_PROC_BIND,$MEMORY_POLICY,$NUMA_NODES,$REPETITION,$TIME,$GFLOPS" >> "$CSV_FILE"
    done
done

echo "completed"
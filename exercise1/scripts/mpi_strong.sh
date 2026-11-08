#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name=gol_mpi_strong
#SBATCH --get-user-env
#SBATCH --chdir=/u/dssc/mpivid00/hpc_2022_final_assignment/exercise1
#SBATCH --nodes=2
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output=gol_mpi_strong_%j.out

set -euo pipefail

ARCH=${1:?Missing architecture}
EVOLUTION=${2:?Missing evolution strategy}
FOG_PROBABILITY=${3:-}

# parameters
GRID_SIZES=${GRID_SIZES:-"10000 20000 30000"}
STEPS=${STEPS:-100}
REPEATS=${REPEATS:-5}

if [[ "$ARCH" == "EPYC" ]]; then
    CORES_PER_SOCKET=64
elif [[ "$ARCH" == "THIN" ]]; then
    CORES_PER_SOCKET=12
else
    echo "Error: architecture must be EPYC or THIN"
    exit 1
fi

CORES_PER_NODE=$((2 * CORES_PER_SOCKET))
NODES=$SLURM_JOB_NUM_NODES
MAX_MPI_TASKS=$((NODES * CORES_PER_NODE))

if [[ "$NODES" -ne 2 ]]; then
    echo "Error: exactly 2 nodes are required"
    exit 1
fi

if [[ "$SLURM_NTASKS" -ne "$MAX_MPI_TASKS" ]]; then
    echo "Error: allocation must reserve all $MAX_MPI_TASKS MPI slots"
    exit 1
fi

if [[ -n "$FOG_PROBABILITY" ]]; then
    FOG_ARGS=(-g "$FOG_PROBABILITY")
    FOG_LABEL="p${FOG_PROBABILITY//./p}"
else
    FOG_ARGS=()
    FOG_LABEL="off"
fi

# modules
module load openMPI/5.0.5

# thread config
export OMP_NUM_THREADS=1
export OMP_DYNAMIC=FALSE

# output
RESULTS_DIR="results/mpi_strong"
mkdir -p "$RESULTS_DIR"

CSV_FILE="${RESULTS_DIR}/gol_mpi_${ARCH}_${NODES}nodes_e${EVOLUTION}_${FOG_LABEL}_job${SLURM_JOB_ID}.csv"

echo "evolution,grid_width,grid_height,steps,mpi_tasks,omp_threads,repetition,read_time,evolution_time,write_time,total_time" > "$CSV_FILE"

# run experiment
echo "Architecture: $ARCH"
echo "Nodes: $NODES"
echo "Maximum MPI tasks: $MAX_MPI_TASKS"
echo "Cores per node: $CORES_PER_NODE"
echo "Evolution: $EVOLUTION"
echo "FoG probability: ${FOG_PROBABILITY:-disabled}"
echo "Grid sizes: $GRID_SIZES"
echo "OpenMP threads: $OMP_NUM_THREADS"
echo "MPI mapping: core"
echo "MPI ranking: fill"
echo "MPI binding: core"
echo "Results: $CSV_FILE"

read -r -a GRID_SIZE_ARRAY <<< "$GRID_SIZES"

for GRID_SIZE in "${GRID_SIZE_ARRAY[@]}"; do
    PATTERN_NAME="grid_${GRID_SIZE}"
    INITIAL_FILE="patterns/output/${PATTERN_NAME}_00000.pgm"

    if [[ ! -f "$INITIAL_FILE" ]]; then
        echo "Error: initial pattern not found: $INITIAL_FILE"
        exit 1
    fi

    for REPETITION in $(seq 1 "$REPEATS"); do
        export BENCHMARK_REPETITION=$REPETITION

        for MPI_TASKS in $(seq 1 "$MAX_MPI_TASKS"); do
            mpirun -np "$MPI_TASKS" --map-by core --bind-to core ./build/gol_parallel -r -f "$PATTERN_NAME" -n "$STEPS" -s 0 -e "$EVOLUTION" -b "${FOG_ARGS[@]}" >> "$CSV_FILE"
        done
    done
done

echo "completed"
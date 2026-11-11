#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name=gol_mpi_weak
#SBATCH --get-user-env
#SBATCH --chdir=/u/dssc/mpivid00/hpc_2022_final_assignment/exercise1
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output=gol_mpi_weak_%j.out

set -euo pipefail

ARCH=${1:?Missing architecture}
EVOLUTION=${2:?Missing evolution strategy}
CONFIG=${3:?Missing configuration (A or B)}
FOG_PROBABILITY=${4:-}

# parameters
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

# scaling configuration
if [[ "$CONFIG" == "A" ]]; then
    NODES=1
    OMP_THREADS=1

    if [[ "$ARCH" == "EPYC" ]]; then
        MPI_TASKS_LIST=(1 2 4 8 16 32 64 128)
        GRID_SIZES=(3000 4243 6000 8485 12000 16971 24000 33941)
    else
        MPI_TASKS_LIST=(1 2 4 8 16 24)
        GRID_SIZES=(3000 4243 6000 8485 12000 14697)
    fi

elif [[ "$CONFIG" == "B" ]]; then
    NODES=2
    OMP_THREADS=$CORES_PER_SOCKET

    MPI_TASKS_LIST=(1 2 3 4)
    GRID_SIZES=(3000 4243 5196 6000)

else
    echo "Error: configuration must be A or B"
    exit 1
fi

if [[ "$SLURM_JOB_NUM_NODES" -ne "$NODES" ]]; then
    echo "Error: configuration $CONFIG requires $NODES nodes"
    exit 1
fi

if [[ "$CONFIG" == "A" ]]; then
    if [[ "$SLURM_NTASKS" -ne "$CORES_PER_NODE" ]]; then
        echo "Error: configuration A requires $CORES_PER_NODE MPI slots"
        exit 1
    fi
else
    if [[ "$SLURM_NTASKS" -ne 4 ]]; then
        echo "Error: configuration B requires 4 MPI slots"
        exit 1
    fi
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
export OMP_NUM_THREADS=$OMP_THREADS
export OMP_DYNAMIC=FALSE

if [[ "$CONFIG" == "B" ]]; then
    export OMP_PLACES=cores
    export OMP_PROC_BIND=spread
fi

# output
RESULTS_DIR="results/mpi_weak"
mkdir -p "$RESULTS_DIR"

CSV_FILE="${RESULTS_DIR}/gol_mpi_${ARCH}_${CONFIG}_${NODES}nodes_e${EVOLUTION}_${FOG_LABEL}_job${SLURM_JOB_ID}.csv"

echo "evolution,grid_width,grid_height,steps,mpi_tasks,omp_threads,repetition,read_time,evolution_time,write_time,total_time" > "$CSV_FILE"

# run experiment
echo "Architecture: $ARCH"
echo "Configuration: $CONFIG"
echo "Nodes: $NODES"
echo "Maximum MPI tasks: ${MPI_TASKS_LIST[-1]}"
echo "Cores per socket: $CORES_PER_SOCKET"
echo "Evolution: $EVOLUTION"
echo "FoG probability: ${FOG_PROBABILITY:-disabled}"
echo "Grid sizes: ${GRID_SIZES[*]}"
echo "OpenMP threads: $OMP_NUM_THREADS"

if [[ "$CONFIG" == "A" ]]; then
    echo "MPI mapping: core"
    echo "MPI binding: core"
else
    echo "MPI mapping: ppr:1:socket:PE=$CORES_PER_SOCKET"
    echo "MPI binding: core"
    echo "OpenMP placement: $OMP_PLACES"
    echo "OpenMP binding: $OMP_PROC_BIND"
fi

echo "Results: $CSV_FILE"

for INDEX in "${!MPI_TASKS_LIST[@]}"; do
    MPI_TASKS=${MPI_TASKS_LIST[$INDEX]}
    GRID_SIZE=${GRID_SIZES[$INDEX]}

    PATTERN_NAME="grid_${GRID_SIZE}"
    INITIAL_FILE="patterns/output/${PATTERN_NAME}_00000.pgm"

    if [[ ! -f "$INITIAL_FILE" ]]; then
        echo "Error: initial pattern not found: $INITIAL_FILE"
        exit 1
    fi

    for REPETITION in $(seq 1 "$REPEATS"); do
        export BENCHMARK_REPETITION=$REPETITION

        if [[ "$CONFIG" == "A" ]]; then
            mpirun -np "$MPI_TASKS" --map-by core --bind-to core ./build/gol_parallel -r -f "$PATTERN_NAME" -n "$STEPS" -s 0 -e "$EVOLUTION" -b "${FOG_ARGS[@]}" >> "$CSV_FILE"
        else
            mpirun -np "$MPI_TASKS" --map-by "ppr:1:socket:PE=${CORES_PER_SOCKET}" --bind-to core ./build/gol_parallel -r -f "$PATTERN_NAME" -n "$STEPS" -s 0 -e "$EVOLUTION" -b "${FOG_ARGS[@]}" >> "$CSV_FILE"
        fi
    done
done

echo "completed"
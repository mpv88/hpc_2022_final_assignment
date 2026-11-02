#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name=gol_omp_scaling
#SBATCH --get-user-env
#SBATCH --chdir=/u/dssc/mpivid00/hpc_2022_final_assignment/exercise1
#SBATCH --exclusive
#SBATCH --ntasks-per-node=2
#SBATCH --ntasks-per-socket=1
#SBATCH --time=02:00:00
#SBATCH --output=gol_omp_scaling_%j.out

set -euo pipefail

ARCH=${1:?Missing architecture}
EVOLUTION=${2:?Missing evolution strategy}
FOG_PROBABILITY=${3:-}

# parameters
GRID_SIZES=${GRID_SIZES:-"10000 20000 30000"}
STEPS=${STEPS:-100}
REPEATS=${REPEATS:-5}

OMP_PLACES=cores
OMP_PROC_BIND=spread

case "$ARCH" in
    EPYC)
        CORES_PER_SOCKET=64
        THREAD_COUNTS=(1 2 4 8 16 32 64)
        ;;
    THIN)
        CORES_PER_SOCKET=12
        THREAD_COUNTS=(1 2 4 8 12)
        ;;
    *)
        echo "Error: architecture must be EPYC or THIN"
        exit 1
        ;;
esac

NODES=$SLURM_JOB_NUM_NODES

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
export OMP_PLACES OMP_PROC_BIND
export OMP_DYNAMIC=FALSE

# output
RESULTS_DIR="results/omp_scalability"
mkdir -p "$RESULTS_DIR"
CSV_FILE="${RESULTS_DIR}/gol_omp_${ARCH}_${NODES}nodes_e${EVOLUTION}_${FOG_LABEL}_job${SLURM_JOB_ID}.csv"

echo "evolution,grid_width,grid_height,steps,mpi_tasks,omp_threads,repetition,read_time,evolution_time,write_time,total_time" > "$CSV_FILE"

# run experiment
echo "Architecture: $ARCH"
echo "Nodes: $NODES"
echo "MPI tasks: $SLURM_NTASKS"
echo "Cores per MPI rank: $CORES_PER_SOCKET"
echo "Evolution: $EVOLUTION"
echo "FoG probability: ${FOG_PROBABILITY:-disabled}"
echo "Grid sizes: $GRID_SIZES"
echo "OpenMP placement: $OMP_PLACES"
echo "OpenMP binding: $OMP_PROC_BIND"
echo "Results: $CSV_FILE"

read -r -a GRID_SIZE_ARRAY <<< "$GRID_SIZES"

for GRID_SIZE in "${GRID_SIZE_ARRAY[@]}"; do
    PATTERN_NAME="grid_${GRID_SIZE}"
    INITIAL_FILE="patterns/output/${PATTERN_NAME}_00000.pgm"

    if [[ ! -f "$INITIAL_FILE" ]]; then
        echo "Error: initial pattern not found: $INITIAL_FILE"
        exit 1
    fi

    # job-specific input name to avoid output-file conflicts
    RUN_PATTERN="${PATTERN_NAME}_job${SLURM_JOB_ID}"
    cp "$INITIAL_FILE" "patterns/output/${RUN_PATTERN}_00000.pgm"

    for REPETITION in $(seq 1 "$REPEATS"); do
        export BENCHMARK_REPETITION=$REPETITION

        for THREADS in "${THREAD_COUNTS[@]}"; do
            export OMP_NUM_THREADS=$THREADS
            mpirun -np "$SLURM_NTASKS" --map-by "ppr:1:socket:PE=${CORES_PER_SOCKET}" --bind-to core ./build/gol_parallel -r -f "$RUN_PATTERN" -n "$STEPS" -s 0 -e "$EVOLUTION" -b "${FOG_ARGS[@]}" >> "$CSV_FILE"
        done
    done
done

echo "completed"
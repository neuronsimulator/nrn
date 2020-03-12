#!/bin/bash
#SBATCH -t 01:00:00
#SBATCH --partition=prod
#SBATCH --exclusive
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=36
#SBATCH --gres=gpu:4

# Stop on error
set -e

# Using top level install directory, set the PYTHONPATH and HOC_LIBRARY_PATH for simulator
# TODO: Change this
INSTALL_DIR=/home/username/..../software/install
export PYTHONPATH=$INSTALL_DIR/RINGTEST
export HOC_LIBRARY_PATH=$INSTALL_DIR/RINGTEST

# Change this according to the desired runtime of the benchmark
# TODO: Change this
export SIM_TIME=100

# Note that if we change NUM_GPUS_PER_NODE then model size changes. So make sure to configure this
# parameter at the begining.
# TODO: Change this
MAX_NUM_GPUS_PER_NODE=4

# Select number of rings of cells according to the number of nodes and gpus
export RINGS_PER_GPU=256

# =============================================================================

# Set the number of cell rings for the circuit according to the Number of Nodes,
# the MAX_NUM_GPUS_PER_NODE and RINGS_PER_GPU
export TOT_NUM_RINGS=$(($SLURM_JOB_NUM_NODES*$(($MAX_NUM_GPUS_PER_NODE*$RINGS_PER_GPU))))

# temporary input directory
INPUT_DATA_DIR=$(mktemp -d $(pwd)/model_data_XXXX)

# Arguments for model building and simulation
RING_CONFIG="-ncell 64 -branch 8 8 -compart 32 32 -npt 32"
NEURON_ARGS="-python $INSTALL_DIR/RINGTEST/ringtest.py -nring $TOT_NUM_RINGS $RING_CONFIG -tstop 0 -coredat $INPUT_DATA_DIR -mpi"
CORENEURON_ARGS="--datpath $INPUT_DATA_DIR --tstop $SIM_TIME --mpi"

###########  GPU EXECUTION ###########
# Generate input data via nrniv and then run simulation using nrniv-core. Simulation time is shown as "Solver Time".
# Note that for GPU execution we typically pefer to run one mpi rank per GPU. So select #ranks and #total ranks accordingly.
# TODO: Change this
NUM_GPUS_PER_NODE=$(($(echo "$CUDA_VISIBLE_DEVICES" | grep -o "," | wc -l)+1))
NUM_RANKS=$(($SLURM_NNODES*$NUM_GPUS_PER_NODE))

srun --nodes=$SLURM_NNODES --ntasks=$NUM_RANKS --ntasks-per-node=$NUM_GPUS_PER_NODE dplace $INSTALL_DIR/NRN/bin/nrniv $NEURON_ARGS
srun --nodes=$SLURM_NNODES --ntasks=$NUM_RANKS --ntasks-per-node=$NUM_GPUS_PER_NODE dplace $INSTALL_DIR/GPU/bin/nrniv-core $CORENEURON_ARGS --gpu

# delete temporary directory
rm -rf $INPUT_DATA_DIR


###########  CPU EXECUTION [OPTIONAL] ###########
# Note that for CPU execution we typically pefer to run one mpi rank per physical core. So select #ranks and #total ranks accordingly.
srun --nodes=$SLURM_NNODES --ntasks=$SLURM_NTASKS --ntasks-per-node=$SLURM_NTASKS_PER_NODE dplace $INSTALL_DIR/NRN/bin/nrniv $NEURON_ARGS
srun --nodes=$SLURM_NNODES --ntasks=$SLURM_NTASKS --ntasks-per-node=$SLURM_NTASKS_PER_NODE dplace $INSTALL_DIR/CPU/bin/nrniv-core $CORENEURON_ARGS

# delete temporary directory
rm -rf $INPUT_DATA_DIR

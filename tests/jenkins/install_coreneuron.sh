#!/usr/bin/bash

set -e
set -x

source ${JENKINS_DIR:-.}/_env_setup.sh

reportinglib_dir=$(spack location --install-dir --latest reportinglib%intel)
libsonata_report_dir=$(spack location --install-dir --latest libsonata-report%intel)

CORENRN_TYPE="$1"

if [ "${CORENRN_TYPE}" = "GPU-non-unified" ] || [ "${CORENRN_TYPE}" = "GPU-unified" ]; then
    # PGI compiler issue in unstable :  BSD-204
    module load nvhpc cuda/11.0.2 hpe-mpi cmake boost
    mkdir build_${CORENRN_TYPE}
else
    module load boost intel hpe-mpi cmake
    export CC=mpicc
    export CXX=mpicxx

    mkdir build_${CORENRN_TYPE} build_intel_${CORENRN_TYPE}
fi

export SALLOC_PARTITION="prod,pre_prod,interactive";

cd $WORKSPACE/build_${CORENRN_TYPE}

echo "${CORENRN_TYPE} build"
if [ "${CORENRN_TYPE}" = "GPU-non-unified" ]; then
    cmake \
        -DCORENRN_ENABLE_GPU=ON \
        -DCORENRN_ENABLE_CUDA_UNIFIED_MEMORY=OFF \
        -DCMAKE_INSTALL_PREFIX=$WORKSPACE/install_${CORENRN_TYPE}/ \
        -DTEST_MPI_EXEC_BIN="srun;--account=proj16;--partition=$SALLOC_PARTITION;--constraint=volta;--gres=gpu:2;--mem;0;-t;00:05:00" \
        -DTEST_EXEC_PREFIX="srun;-n;6;--account=proj16;--partition=$SALLOC_PARTITION;--constraint=volta;--gres=gpu:2;--mem;0;-t;00:05:00" \
        -DAUTO_TEST_WITH_SLURM=OFF \
        -DAUTO_TEST_WITH_MPIEXEC=OFF \
        -DCMAKE_CXX_FLAGS="-D__GCC_ATOMIC_TEST_AND_SET_TRUEVAL=1" \
        $WORKSPACE/
elif [ "${CORENRN_TYPE}" = "GPU-unified" ]; then
    cmake \
        -DCORENRN_ENABLE_GPU=ON \
        -DCORENRN_ENABLE_CUDA_UNIFIED_MEMORY=ON \
        -DCMAKE_INSTALL_PREFIX=$WORKSPACE/install_${CORENRN_TYPE}/ \
        -DTEST_MPI_EXEC_BIN="srun;--account=proj16;--partition=$SALLOC_PARTITION;--constraint=volta;--gres=gpu:2;--mem;0;-t;00:05:00" \
        -DTEST_EXEC_PREFIX="srun;-n;6;--account=proj16;--partition=$SALLOC_PARTITION;--constraint=volta;--gres=gpu:2;--mem;0;-t;00:05:00" \
        -DAUTO_TEST_WITH_SLURM=OFF \
        -DAUTO_TEST_WITH_MPIEXEC=OFF \
        -DCMAKE_CXX_FLAGS="-D__GCC_ATOMIC_TEST_AND_SET_TRUEVAL=1" \
        $WORKSPACE/
elif [ "${CORENRN_TYPE}" = "AoS" ] || [ "${CORENRN_TYPE}" = "SoA" ]; then
    CORENRN_ENABLE_SOA=ON
    ENABLE_OPENMP=ON
    if [ "${CORENRN_TYPE}" = "AoS" ]; then
        CORENRN_ENABLE_SOA=OFF
        ENABLE_OPENMP=OFF
    fi
    cmake  \
      -G 'Unix Makefiles'  \
      -DCMAKE_INSTALL_PREFIX=$WORKSPACE/install_${CORENRN_TYPE}/ \
      -DCMAKE_BUILD_TYPE=Debug  \
      -DCORENRN_ENABLE_SOA=$CORENRN_ENABLE_SOA \
      -DCORENRN_ENABLE_OPENMP=$ENABLE_OPENMP \
      -DCORENRN_ENABLE_REPORTING=ON \
      -DCMAKE_PREFIX_PATH="$reportinglib_dir;$libsonata_report_dir" \
      -DTEST_MPI_EXEC_BIN="mpirun" \
      -DTEST_EXEC_PREFIX="mpirun;-n;2" \
      -DAUTO_TEST_WITH_SLURM=OFF \
      -DAUTO_TEST_WITH_MPIEXEC=OFF \
      $WORKSPACE/
else
    echo "Not a valid CORENRN_TYPE"
    exit 1
fi

make VERBOSE=1 -j8
make install

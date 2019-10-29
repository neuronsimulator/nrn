#!/usr/bin/bash

set -e

. /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh

CORENRN_TYPE="$1"

if [ "${CORENRN_TYPE}" = "GPU-non-unified" ] || [ "${CORENRN_TYPE}" = "GPU-unified" ]; then
    module load pgi/19.4 cuda hpe-mpi cmake

    mkdir build_${CORENRN_TYPE}
else
    module load boost intel hpe-mpi cmake
    export CC=mpicc
    export CXX=mpicxx

    mkdir build_${CORENRN_TYPE} build_intel_${CORENRN_TYPE}
fi

cd $WORKSPACE/build_${CORENRN_TYPE}

echo "${CORENRN_TYPE} build"
if [ "${CORENRN_TYPE}" = "GPU-non-unified" ]; then
    cmake \
        -DCMAKE_C_FLAGS:STRING="-O2" \
        -DCMAKE_CXX_FLAGS:STRING="-O2 -D_GLIBCXX_USE_CXX11_ABI=0 -DR123_USE_SSE=0" \
        -DCOMPILE_LIBRARY_TYPE=STATIC  \
        -DCUDA_HOST_COMPILER=`which gcc` \
        -DCUDA_PROPAGATE_HOST_FLAGS=OFF \
        -DCORENRN_ENABLE_GPU=ON \
        -DCORENRN_ENABLE_CUDA_UNIFIED_MEMORY=OFF \
        -DCMAKE_INSTALL_PREFIX=$WORKSPACE/install_${CORENRN_TYPE}/ \
        -DTEST_MPI_EXEC_BIN="srun;--exclusive;--account=proj16;--partition=interactive;--constraint=volta;--gres=gpu:1;--mem;0;-t;00:05:00" \
        -DTEST_EXEC_PREFIX="srun;-n;6;--exclusive;--account=proj16;--partition=interactive;--constraint=volta;--gres=gpu:1;--mem;0;-t;00:05:00" \
        -DAUTO_TEST_WITH_SLURM=OFF \
        -DAUTO_TEST_WITH_MPIEXEC=OFF \
        $WORKSPACE/
elif [ "${CORENRN_TYPE}" = "GPU-unified" ]; then
    cmake \
        -DCMAKE_C_FLAGS:STRING="-O2" \
        -DCMAKE_CXX_FLAGS:STRING="-O2 -D_GLIBCXX_USE_CXX11_ABI=0 -DR123_USE_SSE=0" \
        -DCOMPILE_LIBRARY_TYPE=STATIC  \
        -DCUDA_HOST_COMPILER=`which gcc` \
        -DCUDA_PROPAGATE_HOST_FLAGS=OFF \
        -DCORENRN_ENABLE_GPU=ON \
        -DCORENRN_ENABLE_CUDA_UNIFIED_MEMORY=ON \
        -DCMAKE_INSTALL_PREFIX=$WORKSPACE/install_${CORENRN_TYPE}/ \
        -DTEST_MPI_EXEC_BIN="srun;--exclusive;--account=proj16;--partition=interactive;--constraint=volta;--gres=gpu:1;--mem;0;-t;00:05:00" \
        -DTEST_EXEC_PREFIX="srun;-n;6;--exclusive;--account=proj16;--partition=interactive;--constraint=volta;--gres=gpu:1;--mem;0;-t;00:05:00" \
        -DAUTO_TEST_WITH_SLURM=OFF \
        -DAUTO_TEST_WITH_MPIEXEC=OFF \
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

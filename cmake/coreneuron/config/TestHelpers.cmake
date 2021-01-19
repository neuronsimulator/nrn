# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# ~~~
# TestHelpers.cmake
# set of Convenience functions for unit testing with cmake
# ~~~

# enable or disable detection of SLURM and MPIEXEC
option(AUTO_TEST_WITH_SLURM "Add srun as test prefix in a SLURM environment" TRUE)
option(AUTO_TEST_WITH_MPIEXEC "Add mpiexec as test prefix in a MPICH2/OpenMPI environment" TRUE)

# ~~~
# Basic SLURM support the prefix "srun" is added to any test in the environment/ For a
# slurm test execution, simply run "salloc [your_exec_parameters] ctest"
# ~~~
if(AUTO_TEST_WITH_SLURM)
  if(NOT DEFINED SLURM_SRUN_COMMAND)
    find_program(
      SLURM_SRUN_COMMAND
      NAMES "srun"
      HINTS "${SLURM_ROOT}/bin" QUIET)
  endif()

  if(SLURM_SRUN_COMMAND)
    set(TEST_EXEC_PREFIX_DEFAULT "${SLURM_SRUN_COMMAND}")
    set(TEST_MPI_EXEC_PREFIX_DEFAULT "${SLURM_SRUN_COMMAND}")
    set(TEST_MPI_EXEC_BIN_DEFAULT "${SLURM_SRUN_COMMAND}")
    set(TEST_WITH_SLURM ON)
  endif()

endif()

# Basic mpiexec support, will just forward mpiexec as prefix
if(AUTO_TEST_WITH_MPIEXEC AND NOT TEST_WITH_SLURM)
  if(NOT DEFINED MPIEXEC)
    find_program(
      MPIEXEC
      NAMES "mpiexec"
      HINTS "${MPI_ROOT}/bin")
  endif()

  if(MPIEXEC)
    set(TEST_MPI_EXEC_PREFIX_DEFAULT "${MPIEXEC}")
    set(TEST_MPI_EXEC_BIN_DEFAULT "${MPIEXEC}")
    set(TEST_WITH_MPIEXEC ON)
  endif()
endif()

# ~~~
# MPI executor program path without arguments used for testing.
# default: srun or mpiexec if found
# ~~~
set(TEST_MPI_EXEC_BIN
    "${TEST_MPI_EXEC_BIN_DEFAULT}"
    CACHE STRING "path of the MPI executor (mpiexec, mpirun) for test execution")

# ~~~
# Test execution prefix. Override this variable for any execution prefix required
# in clustered environment
#
# To specify manually a command with argument, e.g -DTEST_EXEC_PREFIX="/usr/bin/srun;-n;-4"
# for a srun execution with 4 nodes
#
# default: srun if found
# ~~~
set(TEST_EXEC_PREFIX
    "${TEST_EXEC_PREFIX_DEFAULT}"
    CACHE STRING "prefix command for the test executions")

# ~~~
# Test execution prefix specific for MPI programs.
#
# To specify manually a command with argument, use the cmake list syntax. e.g
# -DTEST_EXEC_PREFIX="/usr/bin/mpiexec;-n;-4" for an MPI execution with 4 nodes
#
# default: srun or mpiexec if found
# ~~~
set(TEST_MPI_EXEC_PREFIX
    "${TEST_MPI_EXEC_PREFIX_DEFAULT}"
    CACHE STRING "prefix command for the MPI test executions")

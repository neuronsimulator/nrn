# Includes the install_files function below
include(InstallFiles)

# TEST_LIBRARIES variable is used by the CommonCTest.cmake script to link against the given libraries
set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} coreneuron)

# CommonCTest, in the current folder recursively compiles targets for *.cpp files using TEST_LIBRARIES
#include(CommonCTest)

if(CORENEURON_NEURODAMUS_MECHS AND CORENEURON_MAIN)

  set(SLURM_OPTIONS "--ntasks-per-node=1")
  make_mpi_exeprefix(128 MPI_PREFIX)
  string(REPLACE ";" " " SRUN_PREFIX "${MPI_PREFIX}")

  set(SIM_NAME "128")
  set(TEST_ARGS "--datpath=/gpfs/bbp.cscs.ch/project/proj20/10x10/3m/3columns0 --outpath=${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME} --tstop=50. -mpi")

  configure_file(production_test.sh.in ${SIM_NAME}/production_test.sh @ONLY)

  add_test(NAME ${SIM_NAME}_PRODUCTION_TEST
           COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}/production_test.sh
           WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}")

endif()


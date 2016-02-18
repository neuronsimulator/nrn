# Includes the install_files function below
include(InstallFiles)

# TEST_LIBRARIES variable is used by the CommonCTest.cmake script to link against the given libraries
set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} coreneuron)

# CommonCTest, in the current folder recursively compiles targets for *.cpp files using TEST_LIBRARIES
#include(CommonCTest)

if(CORENEURON_NEURODAMUS_MECHS AND CORENEURON_MAIN)


  function(exec_test SIM_NAME)
    if(${SIM_NAME} MATCHES "93k_3columns_O1_3m")
      make_mpi_exeprefix(128 MPI_PREFIX)
      string(REPLACE ";" " " SRUN_PREFIX "${MPI_PREFIX}")
      set(TEST_ARGS "--datpath=/gpfs/bbp.cscs.ch/project/proj16/coreneuron/data/${SIM_NAME}/128 --outpath=${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME} --tstop=50. -mpi")
    elseif(${SIM_NAME} MATCHES "31k_mc2_column_O1_release_31.05.12")
      make_mpi_exeprefix(16 MPI_PREFIX)
      string(REPLACE ";" " " SRUN_PREFIX "${MPI_PREFIX}")
      set(TEST_ARGS "--datpath=/gpfs/bbp.cscs.ch/project/proj16/coreneuron/data/${SIM_NAME}/16 --outpath=${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME} --forwardskip=5000. --tstop=100. -mpi")
    endif()

    configure_file(production_test.sh.in ${SIM_NAME}/production_test.sh @ONLY)

    add_test(NAME ${SIM_NAME}_PRODUCTION_TEST
             COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}/production_test.sh
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}")
  endfunction(exec_test)

  set(SIM_NAME "93k_3columns_O1_3m" "31k_mc2_column_O1_release_31.05.12")
  foreach(test ${SIM_NAME})
    exec_test(${test})
  endforeach()

endif()


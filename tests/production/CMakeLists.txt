# Copyright (c) 2016, Blue Brain Project
# All rights reserved.

# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.


# TEST_LIBRARIES variable is used by the CommonCTest.cmake script to link against the given libraries
set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} coreneuron)

if(ADDITIONAL_MECHS AND CORENEURON_MAIN)

  function(exec_test SIM_NAME)
    if(${SIM_NAME} MATCHES "93k_3columns_O1_3m")
      string(REPLACE ";" " " SRUN_PREFIX "${TEST_MPI_EXEC_PREFIX};-n;128")
      set(TEST_ARGS "--datpath=/gpfs/bbp.cscs.ch/project/proj16/coreneuron/data/${SIM_NAME}/128 --outpath=${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME} --tstop=100. -mpi")
    elseif(${SIM_NAME} MATCHES "31k_mc2_column_O1_release_31.05.12")
      string(REPLACE ";" " " SRUN_PREFIX "${TEST_MPI_EXEC_PREFIX};-n;16")
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


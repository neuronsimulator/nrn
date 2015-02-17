# This test is for coreneuron havign all the mechanisms integrated into the
# library. Runs when the corresponding condition is utilized plus the
# executable is built.
if(CORENEURON_NEURODAMUS_MECHS AND CORENEURON_MAIN)
  configure_file(10Cells_bgq/10cells_bgq.sh.in ${CMAKE_CURRENT_BINARY_DIR}/10Cells_bgq/10cells_bgq.sh @ONLY)
endif()


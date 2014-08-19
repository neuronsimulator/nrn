# This test is for corebluron havign all the mechanisms integrated into the
# library. Runs when the corresponding condition is utilized plus the
# executable is built.
if(COREBLURON_NEURODAMUS_MECHS AND COREBLURON_MAIN)
  configure_file(10Cells_bgq/10cells_bgq.sh.in ${CMAKE_SOURCE_DIR}/tests/validation/10Cells_bgq/10cells_bgq.sh @ONLY)
endif()


list(APPEND TEST_ENV PATH=${PROJECT_BINARY_DIR}/bin:${PROJECT_SOURCE_DIR}/bin:${PROJECT_SOURCE_DIR}/external/nrntest:$ENV{PATH})
add_test(external_nrntest ${CMAKE_COMMAND} -E env ${TEST_ENV} $ENV{SHELL} ${PROJECT_SOURCE_DIR}/external/tests/nrntest/runtests)

# These produce reference files called hh_sim.csv and so on
set(api_tests ${api_tests_with_reference_files} ${api_tests_without_reference_files})

foreach(api_test_file hh_sim.cpp netcon.cpp sections.c vclamp.cpp)
  string(REPLACE "." "_" api_test_name "${api_test_file}")
  add_executable(${api_test_name} ${api_test_file})
  cpp_cc_configure_sanitizers(TARGET ${api_test_name})
  target_link_libraries(${api_test_name} ${CMAKE_DL_LIBS})
  target_link_libraries(${api_test_name} nrniv_lib)
  target_link_options(${api_test_name} PRIVATE -rdynamic)

  set(test_name "api::${api_test_name}")
  add_test(NAME "${test_name}" COMMAND ${api_test_name})
  set_property(TEST "${test_name}" PROPERTY ENVIRONMENT "${NRN_RUN_FROM_BUILD_DIR_ENV}")
endforeach()

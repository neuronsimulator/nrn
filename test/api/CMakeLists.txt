# These produce reference files called hh_sim.csv and so on
set(api_tests_with_reference_files hh_sim.cpp netcon.cpp vclamp.cpp)
set(api_tests_without_reference_files sections.c)
set(api_tests ${api_tests_with_reference_files} ${api_tests_without_reference_files})
foreach(api_test_file hh_sim.cpp netcon.cpp sections.c vclamp.cpp)
  string(REPLACE "." "_" api_test_name "${api_test_file}")
  add_executable(${api_test_name} ${api_test_file})
  cpp_cc_configure_sanitizers(TARGET ${api_test_name})
  target_link_libraries(${api_test_name} ${CMAKE_DL_LIBS})
  target_link_options(${api_test_name} PRIVATE -rdynamic)
  set(test_name "api::${api_test_name}")
  add_test(NAME "${test_name}" COMMAND ${api_test_name})
  set_property(TEST "${test_name}" PROPERTY ENVIRONMENT "${NRN_RUN_FROM_BUILD_DIR_ENV}")
  if(api_test_file IN_LIST api_tests_with_reference_files)
    get_filename_component(basename "${api_test_file}" NAME_WLE)
    add_test(NAME "${test_name}::comparison"
             COMMAND diff ${basename}.csv ${CMAKE_CURRENT_SOURCE_DIR}/ref/${basename}.csv)
    set_tests_properties("${test_name}::comparison" PROPERTIES DEPENDS "${test_name}")
  endif()
endforeach()

# The netcon.cpp file is a test of doing:
# ~~~
# extern "C" void foo(); // no definition of this is linked
# int main() {
#   ... = dlopen("libnrniv...", ...); // loads definition of foo()
#   foo();
# }
# ~~~
# which requires special options to allow an executable with undefined symbols to be linked.
if(APPLE)
  target_link_options(netcon_cpp PRIVATE -undefined dynamic_lookup)
else()
  target_link_options(netcon_cpp PRIVATE -Wl,--unresolved-symbols=ignore-all)
endif()

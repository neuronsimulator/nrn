foreach(
  api_test_file
  global_scalar.cpp
  hh_sim.cpp
  netcon.cpp
  node_index.cpp
  object_new_nothrow.cpp
  object_new_wrap.cpp
  object_ptr_push.cpp
  runtime_pythonobject_marshaling.cpp
  section_tree.cpp
  sectionlist_to_array.cpp
  segment_diam.cpp
  sections.cpp
  stack_pops.cpp
  symbol_object_string.cpp
  vclamp.cpp)
  string(REPLACE "." "_" api_test_name "${api_test_file}")
  add_executable(${api_test_name} ${api_test_file})
  cpp_cc_configure_sanitizers(TARGET ${api_test_name})
  target_link_libraries(${api_test_name} ${CMAKE_DL_LIBS})
  target_link_libraries(${api_test_name} nrniv_lib)
  target_link_options(${api_test_name} PRIVATE -rdynamic)
  if(NRN_ENABLE_CORENEURON)
    target_compile_definitions(${api_test_name} PUBLIC CORENEURON_ENABLED)
  endif()

  set(test_name "api::${api_test_name}")
  add_test(NAME "${test_name}" COMMAND ${api_test_name})
  set_property(TEST "${test_name}" PROPERTY ENVIRONMENT "${NRN_RUN_FROM_BUILD_DIR_ENV}"
                                            "CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()

# The setpointer tests need mechanisms with a POINTER, which no built-in provides: a density
# mechanism (ptrtest.mod) for nrn_setpointer_pop and a point process (ppptrtest.mod) for
# nrn_pp_setpointer_pop. Generate each from its .mod with nocmodl and compile them into the test
# executable, the same codegen path nrniv uses for its built-in mechanisms.
# NRN_NOCMODL_SANITIZER_ENVIRONMENT disables LeakSanitizer for the nocmodl run (nocmodl leaks, which
# would otherwise fail the generation step under an asan build), matching nocmodl_mod_to_cpp in
# cmake/MacroHelper.cmake.
set(setpointer_gens "")
foreach(setpointer_mod_name ptrtest ppptrtest)
  set(setpointer_mod "${CMAKE_CURRENT_SOURCE_DIR}/${setpointer_mod_name}.mod")
  set(setpointer_mod_copy "${CMAKE_CURRENT_BINARY_DIR}/${setpointer_mod_name}.mod")
  set(setpointer_gen "${CMAKE_CURRENT_BINARY_DIR}/${setpointer_mod_name}.cpp")
  configure_file("${setpointer_mod}" "${setpointer_mod_copy}" COPYONLY)
  add_custom_command(
    OUTPUT ${setpointer_gen}
    COMMAND
      ${CMAKE_COMMAND} -E env "MODLUNIT=${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib"
      "NMODLHOME=${PROJECT_BINARY_DIR}" ${NRN_NOCMODL_SANITIZER_ENVIRONMENT}
      $<TARGET_FILE:${NRN_CODEGENERATOR_TARGET}> ${setpointer_mod_copy} ${NRN_NMODL_--neuron} -o
      ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${NRN_CODEGENERATOR_TARGET} ${setpointer_mod}
            ${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  list(APPEND setpointer_gens ${setpointer_gen})
endforeach()

add_executable(setpointer_cpp setpointer.cpp ${setpointer_gens})
cpp_cc_configure_sanitizers(TARGET setpointer_cpp)
# The generated mechanism includes NEURON internal headers (mech_api.h, the mechanism-cache headers,
# ...); inherit them from nrniv_lib, plus the build's generated-header dir (nrnsemanticversion.h
# etc.), which is a directory-scoped include not carried on the nrniv_lib target.
target_include_directories(setpointer_cpp PRIVATE $<TARGET_PROPERTY:nrniv_lib,INCLUDE_DIRECTORIES>
                                                  ${PROJECT_BINARY_DIR}/include)
target_compile_definitions(setpointer_cpp PRIVATE $<TARGET_PROPERTY:nrniv_lib,COMPILE_DEFINITIONS>)
target_link_libraries(setpointer_cpp ${CMAKE_DL_LIBS})
target_link_libraries(setpointer_cpp nrniv_lib)
target_link_options(setpointer_cpp PRIVATE -rdynamic)
if(NRN_ENABLE_CORENEURON)
  target_compile_definitions(setpointer_cpp PUBLIC CORENEURON_ENABLED)
endif()
add_test(NAME "api::setpointer_cpp" COMMAND setpointer_cpp)
set_property(TEST "api::setpointer_cpp" PROPERTY ENVIRONMENT "${NRN_RUN_FROM_BUILD_DIR_ENV}"
                                                 "CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

# Generate a density mechanism and a point process with NMODL RANDOM variables to exercise both
# public wrapper APIs without Python support.
set(nmodlrandom_gens "")
foreach(nmodlrandom_mod_name randomdensity randompoint)
  set(nmodlrandom_mod "${CMAKE_CURRENT_SOURCE_DIR}/${nmodlrandom_mod_name}.mod")
  set(nmodlrandom_mod_copy "${CMAKE_CURRENT_BINARY_DIR}/${nmodlrandom_mod_name}.mod")
  set(nmodlrandom_gen "${CMAKE_CURRENT_BINARY_DIR}/${nmodlrandom_mod_name}.cpp")
  configure_file("${nmodlrandom_mod}" "${nmodlrandom_mod_copy}" COPYONLY)
  add_custom_command(
    OUTPUT ${nmodlrandom_gen}
    COMMAND
      ${CMAKE_COMMAND} -E env "MODLUNIT=${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib"
      "NMODLHOME=${PROJECT_BINARY_DIR}" ${NRN_NOCMODL_SANITIZER_ENVIRONMENT}
      $<TARGET_FILE:${NRN_CODEGENERATOR_TARGET}> ${nmodlrandom_mod_copy} ${NRN_NMODL_--neuron} -o
      ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${NRN_CODEGENERATOR_TARGET} ${nmodlrandom_mod}
            ${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  list(APPEND nmodlrandom_gens ${nmodlrandom_gen})
endforeach()

add_executable(nmodlrandom_cpp nmodlrandom.cpp ${nmodlrandom_gens})
cpp_cc_configure_sanitizers(TARGET nmodlrandom_cpp)
target_include_directories(nmodlrandom_cpp PRIVATE $<TARGET_PROPERTY:nrniv_lib,INCLUDE_DIRECTORIES>
                                                   ${PROJECT_BINARY_DIR}/include)
target_compile_definitions(nmodlrandom_cpp PRIVATE $<TARGET_PROPERTY:nrniv_lib,COMPILE_DEFINITIONS>)
target_link_libraries(nmodlrandom_cpp ${CMAKE_DL_LIBS})
target_link_libraries(nmodlrandom_cpp nrniv_lib)
target_link_options(nmodlrandom_cpp PRIVATE -rdynamic)
if(NRN_ENABLE_CORENEURON)
  target_compile_definitions(nmodlrandom_cpp PUBLIC CORENEURON_ENABLED)
endif()
add_test(NAME "api::nmodlrandom_cpp" COMMAND nmodlrandom_cpp)
set_property(TEST "api::nmodlrandom_cpp" PROPERTY ENVIRONMENT "${NRN_RUN_FROM_BUILD_DIR_ENV}"
                                                  "CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

include(CMakeFindDependencyMacro)

include("${CMAKE_CURRENT_LIST_DIR}/neuronTargets.cmake")

get_filename_component(_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_prefix "${_dir}/../../.." ABSOLUTE)

set(_NEURON_MAIN "${_prefix}/share/nrn/nrnmain.cpp")
set(_NEURON_MAIN_INCLUDE_DIR "${_prefix}/include/nrncvode")

set(_NEURON_MECH_REG "${_prefix}/share/nrn/mod_reg_nrn.cpp.in")
set(_NEURON_MECH_REG_CORE "${_prefix}/share/nrn/mod_reg_corenrn.cpp.in")

find_program(NOCMODL nocmodl REQUIRED)
find_program(NMODL nmodl REQUIRED)

function(create_libnrnmech)
  cmake_parse_arguments(NRN_MECH "CORENEURON" "" "MOD_FILES" ${ARGN})

  set(TRANSPILER "${NOCMODL}")
  set(RETURN_TYPE "\"C\" void")
  set(PROG_SUFFIX "")
  if(NRN_MECH_CORENEURON)
    set(TRANSPILER "${NEURON}")
    set(RETURN_TYPE "int")
    set(PROG_SUFFIX "-core")
  endif()

  foreach(MOD_FILE IN LISTS NRN_MECH_MOD_FILES)
    # set(MOD_FILE common/mod/DetAMPANMDA.mod)
    string(REGEX REPLACE ".*/\([^/]+\)[.]mod$" "\\1" MOD_STUB "${MOD_FILE}")
    string(REGEX REPLACE "[.]mod$" ".cpp" FULL_CPP_FILE "${MOD_FILE}")
    string(REGEX REPLACE "^.*/" "cpp/" CPP_FILE "${FULL_CPP_FILE}")

    list(APPEND L_MECH_DECLARE "extern ${RETURN_TYPE} _${MOD_STUB}_reg(void)\;")
    list(APPEND L_MECH_PRINT "fprintf(stderr, \" \\\"${MOD_FILE}\\\"\")\;")
    list(APPEND L_MECH_REGISTRE "_${MOD_STUB}_reg()\;")

    add_custom_command(
      COMMAND "${TRANSPILER}" -o "${CMAKE_CURRENT_BINARY_DIR}/cpp"
              "${CMAKE_CURRENT_SOURCE_DIR}/${MOD_FILE}"
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE}")

    list(APPEND L_SOURCES "${CPP_FILE}")
  endforeach()

  list(JOIN L_MECH_DECLARE "\n" MECH_DECLARE)
  list(JOIN L_MECH_PRINT "    \n" MECH_PRINT)
  list(JOIN L_MECH_REGISTRE "  \n" MECH_REGISTRE)

  string(REGEX REPLACE ".*/\([^/]+\).in$" "\\1" MECH_REG "${_NEURON_MECH_REG}")

  configure_file(${_NEURON_MECH_REG} ${MECH_REG} @ONLY)

  add_library(libnrnmech ${L_SOURCES})
  target_link_libraries(libnrnmech neuron::nrniv)

  add_executable(speciale ${_NEURON_MAIN} ${MECH_REG})
  target_include_directories(speciale PUBLIC ${_NEURON_MAIN_INCLUDE_DIR})
  target_link_libraries(speciale libnrnmech)
  install(TARGETS speciale DESTINATION bin)
endfunction()

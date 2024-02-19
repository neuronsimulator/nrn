include(CMakeFindDependencyMacro)

find_dependency(Threads)

include("${CMAKE_CURRENT_LIST_DIR}/neuronTargets.cmake")

get_filename_component(_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_prefix "${_dir}/../../.." ABSOLUTE)

set(_NEURON_MAIN "${_prefix}/share/nrn/nrnmain.cpp")
set(_NEURON_MAIN_INCLUDE_DIR "${_prefix}/include/nrncvode")
set(_NEURON_MECH_REG "${_prefix}/share/nrn/mod_reg_nrn.cpp.in")

set(_CORENEURON_BASE_MOD "${_prefix}/share/modfile")
set(_CORENEURON_MAIN "${_prefix}/share/coreneuron/coreneuron.cpp")
set(_CORENEURON_MECH_REG "${_prefix}/share/nrn/mod_reg_corenrn.cpp.in")
set(_CORENEURON_MECH_ENG "${_prefix}/share/coreneuron/enginemech.cpp")
set(_CORENEURON_RANDOM_INCLUDE "${_prefix}/include/coreneuron/utils/randoms")

find_program(NOCMODL nocmodl REQUIRED)
find_program(NMODL nmodl REQUIRED)

function(create_libnrnmech)
  cmake_parse_arguments(NRN_MECH "CORENEURON" "" "MOD_FILES" ${ARGN})

  set(INPUT_MAIN ${_NEURON_MAIN})
  set(INPUT_MECH_REG ${_NEURON_MECH_REG})
  set(PROG_SUFFIX "")
  set(RETURN_TYPE "\"C\" void")
  set(TRANSPILER "${NOCMODL}")
  set(OUTDIR "neuron")
  if(NRN_MECH_CORENEURON)
    set(INPUT_MAIN ${_CORENEURON_MAIN})
    set(INPUT_MECH_REG ${_CORENEURON_MECH_REG})
    set(PROG_SUFFIX "-core")
    set(RETURN_TYPE "int")
    set(TRANSPILER "${NMODL}")
    set(OUTDIR "coreneuron")
  endif()

  foreach(MOD_FILE IN LISTS NRN_MECH_MOD_FILES)
    string(REGEX REPLACE ".*/\([^/]+\)[.]mod$" "\\1" MOD_STUB "${MOD_FILE}")
    list(APPEND INPUT_STUBS "${MOD_STUB}")
    list(APPEND MOD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${MOD_FILE}")
  endforeach()

  if(NRN_MECH_CORENEURON)
    # CoreNEURON requires additional mod files. Only append them to the input list if similar named
    # mods are _not yet present_
    file(GLOB BASE_MOD_FILES "${_CORENEURON_BASE_MOD}/*.mod")
    foreach(MOD_FILE IN LISTS BASE_MOD_FILES)
      string(REGEX REPLACE ".*/\([^/]+\)[.]mod$" "\\1" MOD_STUB "${MOD_FILE}")
      if("${MOD_STUB}" IN_LIST INPUT_STUBS)

      else()
        list(APPEND MOD_FILES "${MOD_FILE}")
      endif()
    endforeach()
  endif()

  foreach(MOD_FILE IN LISTS MOD_FILES)
    # set(MOD_FILE common/mod/DetAMPANMDA.mod)
    string(REGEX REPLACE ".*/\([^/]+\)[.]mod$" "\\1" MOD_STUB "${MOD_FILE}")
    string(REGEX REPLACE "[.]mod$" ".cpp" FULL_CPP_FILE "${MOD_FILE}")
    string(REGEX REPLACE "^.*/" "${OUTDIR}/cpp/" CPP_FILE "${FULL_CPP_FILE}")

    list(APPEND L_MECH_DECLARE "extern ${RETURN_TYPE} _${MOD_STUB}_reg(void)\;")
    list(APPEND L_MECH_PRINT "fprintf(stderr, \" \\\"${MOD_FILE}\\\"\")\;")
    list(APPEND L_MECH_REGISTRE "_${MOD_STUB}_reg()\;")

    add_custom_command(COMMAND "${TRANSPILER}" -o "${CMAKE_CURRENT_BINARY_DIR}/${OUTDIR}/cpp"
                               "${MOD_FILE}" OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE}")

    list(APPEND L_SOURCES "${CPP_FILE}")
  endforeach()

  list(JOIN L_MECH_DECLARE "\n" MECH_DECLARE)
  list(JOIN L_MECH_PRINT "    \n" MECH_PRINT)
  list(JOIN L_MECH_REGISTRE "  \n" MECH_REGISTRE)

  string(REGEX REPLACE ".*/\([^/]+\).in$" "\\1" MECH_REG "${INPUT_MECH_REG}")

  configure_file(${INPUT_MECH_REG} ${MECH_REG} @ONLY)

  if(NRN_MECH_CORENEURON)
    add_library(libcorenrnmech ${_CORENEURON_MECH_ENG} ${L_SOURCES})
    target_include_directories(libcorenrnmech PRIVATE ${_CORENEURON_RANDOM_INCLUDE})
    target_compile_definitions(libcorenrnmech PRIVATE ADDITIONAL_MECHS NRN_PRCELLSTATE=0
                                                      CORENEURON_BUILD)
    target_link_libraries(libcorenrnmech neuron::corenrn)

    set(MECH_LIB libcorenrnmech)
  else()
    add_library(libnrnmech ${L_SOURCES})
    target_link_libraries(libnrnmech neuron::nrniv)

    set(MECH_LIB libnrnmech)
  endif()

  add_executable(speciale${PROG_SUFFIX} ${INPUT_MAIN} ${MECH_REG})
  target_include_directories(speciale${PROG_SUFFIX} PUBLIC ${_NEURON_MAIN_INCLUDE_DIR})
  target_link_libraries(speciale${PROG_SUFFIX} ${MECH_LIB})
  install(TARGETS speciale${PROG_SUFFIX} DESTINATION bin)
endfunction()

# ~~~
# Helper functions for generating NEURON mechanism libraries directly in CMake
# The basic idea is
# ~~~
function(create_nrnmech)
  set(options NEURON CORENEURON SPECIAL NMODL_NEURON_CODEGEN)
  set(oneValueArgs
      MECHANISM_NAME
      TARGET_LIBRARY_NAME
      TARGET_EXECUTABLE_NAME
      OUTPUT_DIR
      LIBRARY_TYPE
      NOCMODL_EXECUTABLE
      NMODL_EXECUTABLE)
  set(multiValueArgs MOD_FILES NMODL_EXTRA_ARGS EXTRA_ENV)
  cmake_parse_arguments(NRN_MECH "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # The name of the output library
  set(LIBNAME "nrnmech")
  # The name of the output executable
  set(EXENAME "special")
  # The default type of the output library
  set(DEFAULT_LIBRARY_TYPE "SHARED")
  # The default name of the mechanism
  set(DEFAULT_MECHANISM_NAME "neuron")

  if(NRN_MECH_CORENEURON)
    if(NOT NRN_ENABLE_CORENEURON)
      message(FATAL_ERROR "CoreNEURON support not enabled")
    endif()
  endif()

  if(NOT NRN_MECH_NEURON AND NOT NRN_MECH_CORENEURON)
    message(
      FATAL_ERROR
        "No output specified for mod files, please specify at least one of `NEURON` or `CORENEURON` outputs"
    )
  endif()

  if(NRN_MECH_MECHANISM_NAME)
    set(MECHANISM_NAME "${NRN_MECH_MECHANISM_NAME}")
  else()
    set(MECHANISM_NAME "${DEFAULT_MECHANISM_NAME}")
  endif()

  # the `nmodl` and `nocmodl` executables are usually found through `find_program` on the user's
  # system, but we allow overrides (for testing purposes only)
  if(NRN_MECH_NMODL_EXECUTABLE)
    set(NMODL_EXECUTABLE "${NRN_MECH_NMODL_EXECUTABLE}")
  else()
    set(NMODL_EXECUTABLE "${NMODL}")
  endif()

  if(NRN_MECH_NOCMODL_EXECUTABLE)
    set(NOCMODL_EXECUTABLE "${NRN_MECH_NOCMODL_EXECUTABLE}")
  else()
    set(NOCMODL_EXECUTABLE $<TARGET_FILE:neuron::nocmodl>)
  endif()

  # the option `CORENRN_ENABLE_SHARED` toggles the kind of library we want to build, so we respect
  # it here
  if(NRN_MECH_LIBRARY_TYPE)
    set(LIBRARY_TYPE "${NRN_MECH_LIBRARY_TYPE}")
  else()
    set(LIBRARY_TYPE "${DEFAULT_LIBRARY_TYPE}")
  endif()

  # nmodl by default generates code for coreNEURON, so we toggle this via an option
  if(NRN_MECH_NMODL_NEURON_CODEGEN)
    set(NEURON_TRANSPILER_LAUNCHER ${NMODL_EXECUTABLE} --neuron)
  else()
    set(NEURON_TRANSPILER_LAUNCHER ${NOCMODL_EXECUTABLE})
  endif()

  # any extra environment variables that need to be passed (for testing purposes only). Because
  # CMake likes to escape and quote things, we need to do it the roundabout way...
  if(NRN_MECH_EXTRA_ENV)
    set(ENV_COMMAND "${CMAKE_COMMAND}" -E env ${NRN_MECH_EXTRA_ENV})
  else()
    set(ENV_COMMAND)
  endif()

  # Override the _target_ name, but not the library name. This is useful when we are using this
  # function for building NEURON components, since we may experience collisions in the target names
  if(NRN_MECH_TARGET_LIBRARY_NAME)
    set(TARGET_LIBRARY_NAME "${NRN_MECH_TARGET_LIBRARY_NAME}")
  else()
    set(TARGET_LIBRARY_NAME "${LIBNAME}")
  endif()

  # Override the _target_ name, but not the executable name. This is useful when we are using this
  # function for building NEURON components, since we may experience collisions in the target names
  if(NRN_MECH_TARGET_EXECUTABLE_NAME)
    set(TARGET_EXECUTABLE_NAME "${NRN_MECH_TARGET_EXECUTABLE_NAME}")
  else()
    set(TARGET_EXECUTABLE_NAME "${EXENAME}")
  endif()

  # Where to output the intermediate files
  if(NOT NRN_MECH_OUTPUT_DIR)
    set(NRN_MECH_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  # Collect mod files, output any warnings
  foreach(MOD_FILE IN LISTS NRN_MECH_MOD_FILES)
    if(NOT MOD_FILE MATCHES ".*mod$")
      message(WARNING "File ${MOD_FILE} has an extension that is not .mod, compilation may fail")
    endif()
    get_filename_component(MOD_STUB "${MOD_FILE}" NAME_WLE)
    list(APPEND INPUT_STUBS "${MOD_STUB}")
    list(APPEND MOD_FILES "${MOD_FILE}")
  endforeach()

  # Convert mod files for use with NEURON
  if(NRN_MECH_NEURON)
    # Convert to CPP files
    foreach(MOD_FILE IN LISTS MOD_FILES)
      get_filename_component(MOD_STUB "${MOD_FILE}" NAME_WLE)
      # nocmodl has trouble with symlinks, so we always use the real path
      get_filename_component(MOD_ABSPATH "${MOD_FILE}" REALPATH)
      set(CPP_FILE "cpp/${MOD_STUB}.cpp")
      file(RELATIVE_PATH MOD_SHORT "${CMAKE_SOURCE_DIR}" "${MOD_ABSPATH}")

      list(APPEND L_MECH_DECLARE "extern \"C\" void _${MOD_STUB}_reg(void)\;")
      list(APPEND L_MECH_PRINT "fprintf(stderr, \" \\\"${MOD_SHORT}\\\"\")\;")
      list(APPEND L_MECH_REGISTRE "_${MOD_STUB}_reg()\;")

      add_custom_command(
        COMMAND ${ENV_COMMAND} ${NEURON_TRANSPILER_LAUNCHER} -o "${NRN_MECH_OUTPUT_DIR}/cpp"
                "${MOD_ABSPATH}" ${NRN_MECH_NMODL_EXTRA_ARGS}
        OUTPUT "${NRN_MECH_OUTPUT_DIR}/${CPP_FILE}"
        COMMENT "Converting ${MOD_ABSPATH} to ${NRN_MECH_OUTPUT_DIR}/${CPP_FILE}"
        # TODO some mod files may include other files, and NMODL can get the AST of a given file in
        # JSON form, which we could potentially parse with CMake and get the full list of
        # dependencies
        DEPENDS "${MOD_ABSPATH}"
        VERBATIM)

      list(APPEND L_SOURCES "${NRN_MECH_OUTPUT_DIR}/${CPP_FILE}")
    endforeach()

    # add the nrnmech library
    add_library(${TARGET_LIBRARY_NAME} ${LIBRARY_TYPE} ${L_SOURCES})
    set_target_properties(${TARGET_LIBRARY_NAME} PROPERTIES OUTPUT_NAME "${LIBNAME}")
    target_link_libraries(${TARGET_LIBRARY_NAME} PUBLIC neuron::nrniv)

    # we need to add the `mech_func.cpp` file as well since it handles registration of mechanisms
    list(JOIN L_MECH_DECLARE "\n" MECH_DECLARE)
    list(JOIN L_MECH_PRINT "    \n" MECH_PRINT)
    list(JOIN L_MECH_REGISTRE "  \n" MECH_REGISTRE)
    get_filename_component(MECH_REG "${_NEURON_MECH_REG}" NAME_WLE)
    configure_file(${_NEURON_MECH_REG} ${MECH_REG} @ONLY)
    target_sources(${TARGET_LIBRARY_NAME} PRIVATE ${MECH_REG})

    # add the special executable
    if(NRN_MECH_SPECIAL)
      add_executable(${TARGET_EXECUTABLE_NAME} ${_NEURON_MAIN} ${MECH_REG})
      target_include_directories(${TARGET_EXECUTABLE_NAME} PUBLIC ${_NEURON_MAIN_INCLUDE_DIR})
      target_link_libraries(${TARGET_EXECUTABLE_NAME} ${TARGET_LIBRARY_NAME})
      set_target_properties(${TARGET_EXECUTABLE_NAME} PROPERTIES OUTPUT_NAME "special")
    endif()

  endif()

  # Convert mod files for use with coreNEURON
  if(NRN_MECH_CORENEURON)
    # CoreNEURON requires additional mod files. Only append them to the input list if similar named
    # mods are _not yet present_
    file(GLOB BASE_MOD_FILES "${_CORENEURON_BASE_MOD}/*.mod")
    foreach(MOD_FILE IN LISTS BASE_MOD_FILES)
      get_filename_component(MOD_STUB "${MOD_FILE}" NAME_WLE)
      if(NOT "${MOD_STUB}" IN_LIST INPUT_STUBS)
        list(APPEND MOD_FILES "${MOD_FILE}")
      endif()
    endforeach()

    # Convert to CPP files
    foreach(MOD_FILE IN LISTS MOD_FILES)
      get_filename_component(MOD_STUB "${MOD_FILE}" NAME_WLE)
      # nmodl _may_ have trouble with symlinks, so we always use the real path
      get_filename_component(MOD_ABSPATH "${MOD_FILE}" REALPATH)
      set(CPP_FILE "cpp_core/${MOD_STUB}.cpp")
      file(RELATIVE_PATH MOD_SHORT "${CMAKE_SOURCE_DIR}" "${MOD_ABSPATH}")

      list(APPEND L_CORE_MECH_DECLARE "extern int void _${MOD_STUB}_reg(void)\;")
      list(APPEND L_CORE_MECH_PRINT "fprintf(stderr, \" \\\"${MOD_SHORT}\\\"\")\;")
      list(APPEND L_CORE_MECH_REGISTRE "_${MOD_STUB}_reg()\;")

      add_custom_command(
        COMMAND ${ENV_COMMAND} ${NMODL_EXECUTABLE} -o "${NRN_MECH_OUTPUT_DIR}/cpp_core"
                "${MOD_ABSPATH}" ${NRN_MECH_NMODL_EXTRA_ARGS}
        OUTPUT "${NRN_MECH_OUTPUT_DIR}/${CPP_FILE}"
        COMMENT "Converting ${MOD_ABSPATH} to ${NRN_MECH_OUTPUT_DIR}/${CPP_FILE}"
        DEPENDS "${MOD_ABSPATH}"
        VERBATIM)

      list(APPEND L_CORE_SOURCES "${NRN_MECH_OUTPUT_DIR}/${CPP_FILE}")
    endforeach()

    add_library(core${TARGET_LIBRARY_NAME} ${LIBRARY_TYPE} ${_CORENEURON_MECH_ENG}
                                           ${L_CORE_SOURCES})
    set_target_properties(core${TARGET_LIBRARY_NAME} PROPERTIES OUTPUT_NAME "core${LIBNAME}")
    target_include_directories(core${TARGET_LIBRARY_NAME} PRIVATE ${_CORENEURON_RANDOM_INCLUDE})
    target_compile_options(core${TARGET_LIBRARY_NAME} PRIVATE ${_CORENEURON_FLAGS})
    target_link_libraries(core${TARGET_LIBRARY_NAME} PUBLIC neuron::corenrn)

    list(JOIN L_CORE_MECH_DECLARE "\n" MECH_DECLARE)
    list(JOIN L_CORE_MECH_PRINT "    \n" MECH_PRINT)
    list(JOIN L_CORE_MECH_REGISTRE "  \n" MECH_REGISTRE)

    get_filename_component(CORE_MECH_REG "${_NEURON_COREMECH_REG}" NAME_WLE)
    configure_file(${_NEURON_MECH_REG} core${CORE_MECH_REG} @ONLY)

    target_sources(core${TARGET_LIBRARY_NAME} PRIVATE core${CORE_MECH_REG})

    if(NRN_MECH_SPECIAL)
      add_executable(core${TARGET_EXECUTABLE_NAME} ${_CORENEURON_MAIN} core${CORE_MECH_REG})
      target_include_directories(core${TARGET_EXECUTABLE_NAME} PUBLIC ${_NEURON_MAIN_INCLUDE_DIR})
      target_link_libraries(core${TARGET_EXECUTABLE_NAME} core${TARGET_LIBRARY_NAME})
      set_target_properties(core${TARGET_EXECUTABLE_NAME} PROPERTIES OUTPUT_NAME "special-core")
    endif()
  endif()
endfunction()

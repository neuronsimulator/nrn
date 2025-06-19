# ~~~
# See ``docs/dev/nrnivmodl-cmake.rst`` for the description of the API.
# ~~~
function(create_nrnmech)
  set(options NEURON CORENEURON SPECIAL NMODL_NEURON_CODEGEN)
  set(oneValueArgs
      MECHANISM_NAME
      TARGET_LIBRARY_NAME
      TARGET_EXECUTABLE_NAME
      LIBRARY_OUTPUT_DIR
      EXECUTABLE_OUTPUT_DIR
      ARTIFACTS_OUTPUT_DIR
      LIBRARY_TYPE
      NOCMODL_EXECUTABLE
      NMODL_EXECUTABLE)
  set(multiValueArgs MOD_FILES NMODL_NEURON_EXTRA_ARGS NMODL_CORENEURON_EXTRA_ARGS EXTRA_ENV)
  cmake_parse_arguments(NRN_MECH "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # The message priority for logging
  set(MESSAGE_PRIORITY "VERBOSE")

  # The name of the output library
  set(LIBNAME "nrnmech")

  # The name of the output executable
  set(EXENAME "special")

  # The default type of the output library. Note that `nrnmech` and `corenrnmech` are actually
  # module libraries (since they are loaded via `dlopen`-like functionality), but a module library
  # cannot be linked to, and since `special` must link to `nrnmech`, so to avoid having to compile
  # the source files twice, we use a shared library instead.
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

  if(NOT NRN_MECH_MOD_FILES)
    message(FATAL_ERROR "No input mod files specified!")
  endif()

  if(NRN_MECH_NEURON)
    message("${MESSAGE_PRIORITY}" "NEURON | enabled")
  else()
    message("${MESSAGE_PRIORITY}" "NEURON | disabled")
  endif()

  if(NRN_MECH_CORENEURON)
    message("${MESSAGE_PRIORITY}" "coreNEURON | enabled")
  else()
    message("${MESSAGE_PRIORITY}" "coreNEURON | disabled")
  endif()

  if(NRN_MECH_SPECIAL)
    message("${MESSAGE_PRIORITY}" "special executable | enabled")
  else()
    message("${MESSAGE_PRIORITY}" "special executable | disabled")
  endif()

  if(NRN_MECH_MECHANISM_NAME)
    set(MECHANISM_NAME "${NRN_MECH_MECHANISM_NAME}")
  else()
    set(MECHANISM_NAME "${DEFAULT_MECHANISM_NAME}")
  endif()

  message("${MESSAGE_PRIORITY}" "MECHANISM_NAME | ${MECHANISM_NAME}")

  # the `nmodl` and `nocmodl` executables are usually found through `find_program` on the user's
  # system, but we allow overrides (for testing purposes only)
  if(NRN_MECH_NMODL_EXECUTABLE)
    set(NMODL_EXECUTABLE "${NRN_MECH_NMODL_EXECUTABLE}")
  else()
    set(NMODL_EXECUTABLE $<TARGET_FILE:neuron::nmodl>)
  endif()

  message("${MESSAGE_PRIORITY}" "NMODL_EXECUTABLE | ${NMODL_EXECUTABLE}")

  if(NRN_MECH_NOCMODL_EXECUTABLE)
    set(NOCMODL_EXECUTABLE "${NRN_MECH_NOCMODL_EXECUTABLE}")
  else()
    set(NOCMODL_EXECUTABLE $<TARGET_FILE:neuron::nocmodl>)
  endif()

  message("${MESSAGE_PRIORITY}" "NOCMODL_EXECUTABLE | ${NOCMODL_EXECUTABLE}")

  # the option `CORENRN_ENABLE_SHARED` toggles the kind of library we want to build, so we respect
  # it here
  if(NRN_MECH_LIBRARY_TYPE)
    set(LIBRARY_TYPE "${NRN_MECH_LIBRARY_TYPE}")
  else()
    set(LIBRARY_TYPE "${DEFAULT_LIBRARY_TYPE}")
  endif()

  message("${MESSAGE_PRIORITY}" "LIBRARY_TYPE | ${LIBRARY_TYPE}")

  # nmodl by default generates code for coreNEURON, so we toggle this via an option
  if(NRN_MECH_NMODL_NEURON_CODEGEN)
    set(NEURON_TRANSPILER_LAUNCHER ${NMODL_EXECUTABLE} --neuron)
  else()
    set(NEURON_TRANSPILER_LAUNCHER ${NOCMODL_EXECUTABLE})
  endif()

  # raise warning that NMODL extra args will be ignored if we use NEURON codegen with NOCMODL
  if(NRN_MECH_NEURON
     AND NOT NRN_MECH_NMODL_NEURON_CODEGEN
     AND NRN_MECH_NMODL_NEURON_EXTRA_ARGS)
    message(
      WARNING
        "${CMAKE_CURRENT_FUNCTION}: requested NEURON library with NOCMODL codegen, but NMODL_NEURON_EXTRA_ARGS is not empty; "
        "will ignore NMODL_NEURON_EXTRA_ARGS when building NEURON library.\n"
        "Hint: if you want to use NMODL for codegen for NEURON, add the NMODL_NEURON_CODEGEN option when calling this function."
    )
    set(NRN_MECH_NMODL_NEURON_EXTRA_ARGS)
  endif()

  list(JOIN NRN_MECH_NMODL_NEURON_EXTRA_ARGS "" NMODL_NEURON_EXTRA_ARGS_SPACES)
  message("${MESSAGE_PRIORITY}" "NMODL_NEURON_EXTRA_ARGS | ${NMODL_NEURON_EXTRA_ARGS_SPACES}")

  # set default flags for NMODL for coreNEURON.
  if(NOT NRN_MECH_NMODL_CORENEURON_EXTRA_ARGS)
    set(NRN_MECH_NMODL_CORENEURON_EXTRA_ARGS passes --inline)
    list(APPEND NRN_MECH_NMODL_CORENEURON_EXTRA_ARGS host --c)
    # OpenACC flags
    if(CMAKE_CUDA_COMPILER)
      list(APPEND NRN_MECH_NMODL_CORENEURON_EXTRA_ARGS acc --oacc)
    endif()
  endif()

  list(JOIN NRN_MECH_NMODL_CORENEURON_EXTRA_ARGS " " NMODL_CORENEURON_EXTRA_ARGS_SPACES)
  message("${MESSAGE_PRIORITY}"
          "NMODL_CORENEURON_EXTRA_ARGS | ${NMODL_CORENEURON_EXTRA_ARGS_SPACES}")

  # any extra environment variables that need to be passed (for testing purposes only). Because
  # CMake likes to escape and quote things, we need to do it the roundabout way...
  if(NRN_MECH_EXTRA_ENV)
    set(ENV_COMMAND "${CMAKE_COMMAND}" -E env ${NRN_MECH_EXTRA_ENV})
  else()
    set(ENV_COMMAND)
  endif()

  message("${MESSAGE_PRIORITY}" "EXTRA_ENV | ${NRN_MECH_EXTRA_ENV}")

  # nocmodl sometimes does not work due to lack of MODLUNIT, see:
  # https://github.com/neuronsimulator/nrn/issues/3470
  if(DEFINED ENV{MODLUNIT})
    list(APPEND ENV_COMMAND "MODLUNIT=$ENV{MODLUNIT}")
  endif()

  # Override the _target_ name, but not the library name. This is useful when we are using this
  # function for building NEURON components, since we may experience collisions in the target names
  if(NRN_MECH_TARGET_LIBRARY_NAME)
    set(TARGET_LIBRARY_NAME "${NRN_MECH_TARGET_LIBRARY_NAME}")
  else()
    set(TARGET_LIBRARY_NAME "${LIBNAME}")
  endif()

  message("${MESSAGE_PRIORITY}" "TARGET_LIBRARY_NAME | ${TARGET_LIBRARY_NAME}")

  # Override the _target_ name, but not the executable name. This is useful when we are using this
  # function for building NEURON components, since we may experience collisions in the target names
  if(NRN_MECH_TARGET_EXECUTABLE_NAME)
    set(TARGET_EXECUTABLE_NAME "${NRN_MECH_TARGET_EXECUTABLE_NAME}")
  else()
    set(TARGET_EXECUTABLE_NAME "${EXENAME}")
  endif()

  message("${MESSAGE_PRIORITY}" "TARGET_EXECUTABLE_NAME | ${TARGET_EXECUTABLE_NAME}")

  # Where to output the library (during build)
  if(NRN_MECH_LIBRARY_OUTPUT_DIR)
    set(LIBRARY_OUTPUT_DIR "${NRN_MECH_LIBRARY_OUTPUT_DIR}")
  else()
    set(LIBRARY_OUTPUT_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
  endif()

  message("${MESSAGE_PRIORITY}" "LIBRARY_OUTPUT_DIR | ${LIBRARY_OUTPUT_DIR}")

  # Where to output the executable (during build)
  if(NRN_MECH_EXECUTABLE_OUTPUT_DIR)
    set(EXECUTABLE_OUTPUT_DIR "${NRN_MECH_EXECUTABLE_OUTPUT_DIR}")
  else()
    set(EXECUTABLE_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  endif()

  message("${MESSAGE_PRIORITY}" "EXECUTABLE_OUTPUT_DIR | ${EXECUTABLE_OUTPUT_DIR}")

  # Where the intermediate CPP files will be placed
  if(NRN_MECH_ARTIFACTS_OUTPUT_DIR)
    set(ARTIFACTS_OUTPUT_DIR "${NRN_MECH_ARTIFACTS_OUTPUT_DIR}")
  else()
    set(ARTIFACTS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  message("${MESSAGE_PRIORITY}" "ARTIFACTS_OUTPUT_DIR | ${ARTIFACTS_OUTPUT_DIR}")

  # Collect mod files, output any warnings
  set(MOD_FILES "")
  foreach(MOD_FILE IN LISTS NRN_MECH_MOD_FILES)
    if(NOT MOD_FILE MATCHES ".*mod$")
      message(WARNING "File ${MOD_FILE} has an extension that is not .mod, compilation may fail")
    endif()
    get_filename_component(MOD_STUB "${MOD_FILE}" NAME_WLE)
    list(APPEND INPUT_STUBS "${MOD_STUB}")
    list(APPEND MOD_FILES "${MOD_FILE}")
  endforeach()

  message("${MESSAGE_PRIORITY}" "MOD_FILES | ${MOD_FILES}")

  # We later include the directories where the mod files are in case people add headers in VERBATIM
  # blocks
  set(MOD_DIRECTORIES)

  # Convert mod files for use with NEURON
  if(NRN_MECH_NEURON)
    # Convert to CPP files
    foreach(MOD_FILE IN LISTS MOD_FILES)
      get_filename_component(MOD_STUB "${MOD_FILE}" NAME_WLE)
      # nocmodl has trouble with symlinks, so we always use the real path
      get_filename_component(MOD_ABSPATH "${MOD_FILE}" REALPATH)
      get_filename_component(MOD_DIRECTORY "${MOD_ABSPATH}" DIRECTORY)
      if(NOT MOD_DIRECTORY IN_LIST MOD_DIRECTORIES)
        list(APPEND MOD_DIRECTORIES "${MOD_DIRECTORY}")
      endif()
      set(CPP_FILE "cpp/${MOD_STUB}.cpp")
      file(RELATIVE_PATH MOD_SHORT "${CMAKE_SOURCE_DIR}" "${MOD_ABSPATH}")

      list(APPEND L_MECH_DECLARE "extern \"C\" void _${MOD_STUB}_reg(void)\;")
      list(APPEND L_MECH_PRINT "fprintf(stderr, \" \\\"${MOD_SHORT}\\\"\")\;")
      list(APPEND L_MECH_REGISTRE "_${MOD_STUB}_reg()\;")

      add_custom_command(
        COMMAND ${ENV_COMMAND} ${NEURON_TRANSPILER_LAUNCHER} -o "${ARTIFACTS_OUTPUT_DIR}/cpp"
                "${MOD_ABSPATH}" ${NRN_MECH_NMODL_NEURON_EXTRA_ARGS}
        OUTPUT "${ARTIFACTS_OUTPUT_DIR}/${CPP_FILE}"
        COMMENT "Converting ${MOD_ABSPATH} to ${ARTIFACTS_OUTPUT_DIR}/${CPP_FILE}"
        # TODO some mod files may include other files, and NMODL can get the AST of a given file in
        # JSON form, which we could potentially parse with CMake and get the full list of
        # dependencies
        DEPENDS "${MOD_ABSPATH}"
        VERBATIM)

      list(APPEND L_SOURCES "${ARTIFACTS_OUTPUT_DIR}/${CPP_FILE}")
    endforeach()

    # add the nrnmech library
    add_library(${TARGET_LIBRARY_NAME} ${LIBRARY_TYPE} ${L_SOURCES})
    set_target_properties(
      ${TARGET_LIBRARY_NAME} PROPERTIES OUTPUT_NAME "${LIBNAME}" LIBRARY_OUTPUT_DIRECTORY
                                                                 "${LIBRARY_OUTPUT_DIR}")
    target_link_libraries(${TARGET_LIBRARY_NAME} PUBLIC neuron::nrniv)

    # we need to add the `mech_func.cpp` file as well since it handles registration of mechanisms
    list(JOIN L_MECH_DECLARE "\n" MECH_DECLARE)
    list(JOIN L_MECH_PRINT "    \n" MECH_PRINT)
    list(JOIN L_MECH_REGISTRE "  \n" MECH_REGISTRE)
    get_filename_component(MECH_REG "${_NEURON_MECH_REG}" NAME_WLE)
    configure_file(${_NEURON_MECH_REG} "${ARTIFACTS_OUTPUT_DIR}/${MECH_REG}" @ONLY)
    target_sources(${TARGET_LIBRARY_NAME} PRIVATE "${ARTIFACTS_OUTPUT_DIR}/${MECH_REG}")
    target_compile_definitions(${TARGET_LIBRARY_NAME} PUBLIC AUTO_DLOPEN_NRNMECH=0)
    target_include_directories(${TARGET_LIBRARY_NAME} BEFORE PUBLIC ${_NEURON_MAIN_INCLUDE_DIR})
    # sometimes people will add `#include`s in VERBATIM blocks; usually those are in the same
    # directory as the mod file, so let's add that as well
    target_include_directories(${TARGET_LIBRARY_NAME} PRIVATE "${MOD_DIRECTORIES}")

    # add the special executable
    if(NRN_MECH_SPECIAL)
      add_executable(${TARGET_EXECUTABLE_NAME} ${_NEURON_MAIN}
                                               "${ARTIFACTS_OUTPUT_DIR}/${MECH_REG}")
      target_include_directories(${TARGET_EXECUTABLE_NAME} BEFORE
                                 PUBLIC ${_NEURON_MAIN_INCLUDE_DIR})
      target_link_libraries(${TARGET_EXECUTABLE_NAME} PUBLIC ${TARGET_LIBRARY_NAME})
      set_target_properties(
        ${TARGET_EXECUTABLE_NAME} PROPERTIES OUTPUT_NAME "special" RUNTIME_OUTPUT_DIRECTORY
                                                                   "${EXECUTABLE_OUTPUT_DIR}")
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

      list(APPEND L_CORE_MECH_DECLARE "extern int _${MOD_STUB}_reg(void)\;")
      list(APPEND L_CORE_MECH_PRINT "fprintf(stderr, \" \\\"${MOD_SHORT}\\\"\")\;")
      list(APPEND L_CORE_MECH_REGISTRE "_${MOD_STUB}_reg()\;")

      add_custom_command(
        COMMAND ${ENV_COMMAND} ${NMODL_EXECUTABLE} -o "${ARTIFACTS_OUTPUT_DIR}/cpp_core"
                "${MOD_ABSPATH}" ${NRN_MECH_NMODL_CORENEURON_EXTRA_ARGS}
        OUTPUT "${ARTIFACTS_OUTPUT_DIR}/${CPP_FILE}"
        COMMENT "Converting ${MOD_ABSPATH} to ${ARTIFACTS_OUTPUT_DIR}/${CPP_FILE}"
        DEPENDS "${MOD_ABSPATH}"
        VERBATIM)

      list(APPEND L_CORE_SOURCES "${ARTIFACTS_OUTPUT_DIR}/${CPP_FILE}")
    endforeach()

    add_library(core${TARGET_LIBRARY_NAME} ${LIBRARY_TYPE} ${_CORENEURON_MECH_ENG}
                                           ${L_CORE_SOURCES})
    set_target_properties(
      core${TARGET_LIBRARY_NAME} PROPERTIES OUTPUT_NAME "core${LIBNAME}" LIBRARY_OUTPUT_DIRECTORY
                                                                         "${LIBRARY_OUTPUT_DIR}")
    target_include_directories(core${TARGET_LIBRARY_NAME} BEFORE
                               PUBLIC ${_CORENEURON_RANDOM_INCLUDE})
    target_compile_options(core${TARGET_LIBRARY_NAME} BEFORE PRIVATE ${_CORENEURON_FLAGS})
    target_link_libraries(core${TARGET_LIBRARY_NAME} PUBLIC neuron::corenrn)
    target_compile_definitions(core${TARGET_LIBRARY_NAME} PUBLIC ADDITIONAL_MECHS)

    if(CMAKE_CUDA_COMPILER)
      # Find the cuda toolkit and openacc (if not found already)
      if(NOT CUDAToolkit_FOUND)
        find_package(CUDAToolkit ${_CORENEURON_MIN_CUDA_TOOLKIT_VERSION} REQUIRED)
      endif()
      if(NOT OpenACC_FOUND)
        find_package(OpenACC REQUIRED)
      endif()
      # Random123 does not play nicely with NVHPC
      target_compile_definitions(core${TARGET_LIBRARY_NAME} PUBLIC R123_USE_INTRIN_H=0)
      # if using NVHPC, link corenrnmech lib to the CUDA runtime and openacc
      if("${LIBRARY_TYPE}" STREQUAL "STATIC")
        target_link_libraries(core${TARGET_LIBRARY_NAME} PUBLIC CUDA::cudart_static
                                                                OpenACC::OpenACC_CXX)
      elseif("${LIBRARY_TYPE}" STREQUAL "SHARED")
        target_link_libraries(core${TARGET_LIBRARY_NAME} PUBLIC CUDA::cudart OpenACC::OpenACC_CXX)
      else()
        message(FATAL_ERROR "Unsupported library type for CUDA: ${LIBRARY_TYPE}")
      endif()
      # for some reason we need to add `-cuda` to nrnmech (which gets propagated to special-core)
      target_link_options(core${TARGET_LIBRARY_NAME} PUBLIC "-cuda")
    endif()

    list(JOIN L_CORE_MECH_DECLARE "\n" MECH_DECLARE)
    list(JOIN L_CORE_MECH_PRINT "    \n" MECH_PRINT)
    list(JOIN L_CORE_MECH_REGISTRE "  \n" MECH_REGISTRE)

    get_filename_component(CORE_MECH_REG "${_CORENEURON_MECH_REG}" NAME_WLE)
    configure_file(${_CORENEURON_MECH_REG} "${ARTIFACTS_OUTPUT_DIR}/core${CORE_MECH_REG}" @ONLY)

    target_sources(core${TARGET_LIBRARY_NAME}
                   PRIVATE "${ARTIFACTS_OUTPUT_DIR}/core${CORE_MECH_REG}")

    if(NRN_MECH_SPECIAL)
      add_executable(core${TARGET_EXECUTABLE_NAME} ${_CORENEURON_MAIN}
                                                   "${ARTIFACTS_OUTPUT_DIR}/core${CORE_MECH_REG}")
      target_include_directories(core${TARGET_EXECUTABLE_NAME} BEFORE
                                 PUBLIC ${_NEURON_MAIN_INCLUDE_DIR})
      target_link_libraries(core${TARGET_EXECUTABLE_NAME} PUBLIC core${TARGET_LIBRARY_NAME})
      target_compile_definitions(core${TARGET_EXECUTABLE_NAME} PUBLIC ADDITIONAL_MECHS)
      set_target_properties(
        core${TARGET_EXECUTABLE_NAME}
        PROPERTIES OUTPUT_NAME "special-core" RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_DIR}")
    endif()
  endif()
endfunction()

# ~~~
#
function(create_nrnmech)
  set(options CORENEURON INSTALL_CPP INSTALL_MOD SPECIAL)
  set(oneValueArgs MECHANISM_NAME TARGET_NAME OUTPUT_DIR)
  cmake_parse_arguments(NRN_MECH "${options}" "${oneValueArgs}" "MOD_FILES" ${ARGN})

  if(NRN_MECH_CORENEURON)
    if(NOT NRN_ENABLE_CORENEURON)
      message(FATAL_ERROR "CoreNEURON support not enabled")
    endif()
  endif()

  if(NOT MECHANISM_NAME)
    set(MECHANISM_NAME neuron)
  endif()

  # Where to output the mod files
  if(NOT OUTPUT_DIR)
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  set(LIBNAME "nrnmech")
  set(EXENAME "special")

  foreach(MOD_FILE IN LISTS NRN_MECH_MOD_FILES)
    get_filename_component(MOD_STUB "${MOD_FILE}" NAME_WLE)
    list(APPEND INPUT_STUBS "${MOD_STUB}")
    list(APPEND MOD_FILES "${MOD_FILE}")
  endforeach()

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
      COMMAND neuron::nocmodl -o "${OUTPUT_DIR}/cpp" "${MOD_ABSPATH}"
      OUTPUT "${OUTPUT_DIR}/${CPP_FILE}"
      DEPENDS neuron::nocmodl "${MOD_ABSPATH}")

    list(APPEND L_SOURCES "${OUTPUT_DIR}/${CPP_FILE}")
  endforeach()

  if(NRN_MECH_CORENEURON)
    # CoreNEURON requires additional mod files. Only append them to the input list if similar named
    # mods are _not yet present_
    file(GLOB BASE_MOD_FILES "${_CORENEURON_BASE_MOD}/*.mod")
    foreach(MOD_FILE IN LISTS BASE_MOD_FILES)
      get_filename_component(MOD_STUB "${MOD_FILE}" NAME_WLE)
      if("${MOD_STUB}" IN_LIST INPUT_STUBS)

      else()
        list(APPEND MOD_FILES "${MOD_FILE}")
      endif()
    endforeach()

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
        COMMAND "${NMODL}" -o "${OUTPUT_DIR}/cpp_core" "${MOD_ABSPATH}"
        OUTPUT "${OUTPUT_DIR}/${CPP_FILE}"
        DEPENDS "${NMODL}" "${MOD_ABSPATH}")

      list(APPEND L_CORE_SOURCES "${OUTPUT_DIR}/${CPP_FILE}")
    endforeach()
  endif()

  # Override the target name, but not the library name. This is useful when we are using this
  # function in NEURON itself, since we may experience collisions in the target names
  if(NRN_MECH_TARGET_NAME)
    set(TARGET_NAME "${NRN_MECH_TARGET_NAME}")
  else()
    set(TARGET_NAME "${LIBNAME}")
  endif()
  add_library(${TARGET_NAME} SHARED ${L_SOURCES})
  set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "${LIBNAME}")
  target_link_libraries(${TARGET_NAME} PUBLIC neuron::nrniv)
  # set_target_properties(${LIBNAME} PROPERTIES OUTPUT_NAME
  # "${LIBNAME}$<$<BOOL:${NRN_MECH_MECHANISM_NAME}>:_${NRN_MECH_MECHANISM_NAME}>")
  install(TARGETS ${TARGET_NAME} DESTINATION lib)

  if(NRN_MECH_CORENEURON)
    add_library(core${TARGET_NAME} SHARED ${_CORENEURON_MECH_ENG} ${L_CORE_SOURCES})
    set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "core${LIBNAME}")
    target_include_directories(core${TARGET_NAME} PRIVATE ${_CORENEURON_RANDOM_INCLUDE})
    target_compile_options(core${TARGET_NAME} PRIVATE ${_CORENEURON_FLAGS})
    target_link_libraries(core${TARGET_NAME} PUBLIC neuron::corenrn)
    # set_target_properties(${LIBNAME} PROPERTIES OUTPUT_NAME
    # "${LIBNAME}$<$<BOOL:${NRN_MECH_MECHANISM_NAME}>:_${NRN_MECH_MECHANISM_NAME}>")
    install(TARGETS core${TARGET_NAME} DESTINATION lib)
  endif()

  if(NRN_MECH_INSTALL_CPP)
    install(FILES ${L_SOURCES} DESTINATION "share/${NRN_MECH_MECHANISM_NAME}/cpp")
    if(NRN_ENABLE_CORENEURON)
      install(FILES ${L_CORE_SOURCES} DESTINATION "share/${NRN_MECH_MECHANISM_NAME}/cpp_core")
    endif()
  endif()

  if(NRN_MECH_INSTALL_MOD)
    install(FILES ${MOD_FILES} DESTINATION "share/${NRN_MECH_MECHANISM_NAME}/mod")
  endif()

  if(NRN_MECH_SPECIAL)
    list(JOIN L_MECH_DECLARE "\n" MECH_DECLARE)
    list(JOIN L_MECH_PRINT "    \n" MECH_PRINT)
    list(JOIN L_MECH_REGISTRE "  \n" MECH_REGISTRE)

    get_filename_component(MECH_REG "${_NEURON_MECH_REG}" NAME_WLE)
    configure_file(${_NEURON_MECH_REG} ${MECH_REG} @ONLY)

    add_executable(${EXENAME} ${_NEURON_MAIN} ${MECH_REG})
    target_include_directories(${EXENAME} PUBLIC ${_NEURON_MAIN_INCLUDE_DIR})
    target_link_libraries(${EXENAME} ${LIBNAME})
    set_target_properties(${EXENAME} PROPERTIES OUTPUT_NAME "special")
    install(TARGETS ${EXENAME} DESTINATION bin)

    if(NRN_MECH_CORENEURON)
      list(JOIN L_CORE_MECH_DECLARE "\n" MECH_DECLARE)
      list(JOIN L_CORE_MECH_PRINT "    \n" MECH_PRINT)
      list(JOIN L_CORE_MECH_REGISTRE "  \n" MECH_REGISTRE)

      get_filename_component(CORE_MECH_REG "${_NEURON_COREMECH_REG}" NAME_WLE)
      configure_file(${_NEURON_MECH_REG} core${CORE_MECH_REG} @ONLY)

      add_executable(core${EXENAME} ${_CORENEURON_MAIN} core${CORE_MECH_REG})
      target_include_directories(core${EXENAME} PUBLIC ${_NEURON_MAIN_INCLUDE_DIR})
      target_link_libraries(core${EXENAME} core${LIBNAME})
      set_target_properties(core${EXENAME} PROPERTIES OUTPUT_NAME "special-core")
      install(TARGETS core${EXENAME} DESTINATION bin)
    endif()
  endif()
endfunction()

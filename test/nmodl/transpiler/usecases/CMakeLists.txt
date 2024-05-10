set(NMODL_USECASE_DIRS
    cnexp_scalar
    cnexp_array
    func_proc
    func_proc_pnt
    global_breakpoint
    hodgkin_huxley
    nonspecific_current
    net_receive
    point_process
    parameter)

file(GLOB NMODL_GOLDEN_REFERENCES "${CMAKE_CURRENT_SOURCE_DIR}/references/*")
if(NMODL_GOLDEN_REFERENCES STREQUAL "")
  cpp_cc_init_git_submodule(${CMAKE_CURRENT_SOURCE_DIR}/references)
endif()
unset(NMODL_GOLDEN_REFERNCES)

add_custom_target(generate_references)

foreach(usecase ${NMODL_USECASE_DIRS})
  # Non-existant dependencies are a way of unconditionally running commands in CMake.
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/references/${usecase}/_does_not_exist_weiohbge)
    message(
      FATAL_ERROR
        "The file: '${CMAKE_CURRENT_SOURCE_DIR}/references/${usecase}/_does_not_exist_weiohbge' must not exist."
    )
  endif()

  add_test(NAME usecase_${usecase}
           COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh ${CMAKE_BINARY_DIR}/bin/nmodl
                   ${CMAKE_CURRENT_SOURCE_DIR}/${usecase})

  add_test(NAME golden_${usecase}
           COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_references.sh ${CMAKE_BINARY_DIR}/bin/nmodl
                   ${CMAKE_CURRENT_SOURCE_DIR}/${usecase})

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/references/${usecase}/_does_not_exist_weiohbge
    DEPENDS nmodl
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_references.sh ${CMAKE_BINARY_DIR}/bin/nmodl
            ${CMAKE_CURRENT_SOURCE_DIR}/${usecase})

  add_custom_target(
    generate_${usecase}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/references/${usecase}/_does_not_exist_weiohbge)
  add_dependencies(generate_references generate_${usecase})
endforeach()

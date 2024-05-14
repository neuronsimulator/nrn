set(NMODL_USECASE_DIRS
    cnexp_scalar
    cnexp_array
    func_proc
    func_proc_pnt
    global_breakpoint
    hodgkin_huxley
    nonspecific_current
    net_receive
    net_send
    point_process
    parameter)

foreach(usecase ${NMODL_USECASE_DIRS})
  add_test(NAME usecase_${usecase}
           COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh ${CMAKE_BINARY_DIR}/bin/nmodl
                   ${CMAKE_CURRENT_SOURCE_DIR}/${usecase})
endforeach()

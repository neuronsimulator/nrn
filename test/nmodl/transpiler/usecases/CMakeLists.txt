set(NMODL_USECASE_DIRS
    solve
    builtin_functions
    constant
    electrode_current
    constructor
    function
    procedure
    global
    hodgkin_huxley
    kinetic
    linear
    morphology
    nonspecific_current
    neuron_variables
    net_event
    net_move
    net_receive
    net_send
    point_process
    parameter
    random
    suffix
    state
    steady_state
    table
    useion
    at_time)

foreach(usecase ${NMODL_USECASE_DIRS})
  add_test(NAME usecase_${usecase}
           COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh ${CMAKE_BINARY_DIR}/bin/nmodl
                   ${CMAKE_CURRENT_SOURCE_DIR}/${usecase})
endforeach()

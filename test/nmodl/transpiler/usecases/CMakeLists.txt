set(NMODL_USECASE_DIRS
    at_time
    builtin_functions
    constant
    constructor
    cvode
    electrode_current
    external
    function
    function_table
    global
    hodgkin_huxley
    kinetic
    linear
    longitudinal_diffusion
    morphology
    net_event
    net_move
    net_receive
    net_send
    neuron_variables
    nonlinear
    nonspecific_current
    parameter
    point_process
    pointer
    procedure
    protect
    random
    solve
    state
    steady_state
    suffix
    table
    useion
    verbatim)

foreach(usecase ${NMODL_USECASE_DIRS})
  make_directory(${CMAKE_CURRENT_BINARY_DIR}/${usecase})
  add_test(
    NAME usecase_${usecase}
    COMMAND
      ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin:$ENV{PATH}"
      "NRNHOME=${CMAKE_BINARY_DIR}" "NEURONHOME=${CMAKE_BINARY_DIR}/share/nrn"
      "CORENRNHOME=${CMAKE_BINARY_DIR}" "NMODL_PYLIB=${PYTHON_LIBRARY}"
      "NMODLHOME=${CMAKE_BINARY_DIR}" "PYTHONPATH=${CMAKE_BINARY_DIR}/lib/python:$ENV{PYTHONPATH}"
      ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh ${CMAKE_BINARY_DIR}/bin/nmodl
      ${CMAKE_CURRENT_SOURCE_DIR}/${usecase}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${usecase})
endforeach()

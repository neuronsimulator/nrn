include("${PROJECT_SOURCE_DIR}/cmake/NeuronTestHelper.cmake")
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
  file(GLOB modfiles "${usecase}/*.mod")
  # build modfiles with NOCMODL
  create_nrnmech(
    NEURON
    MOD_FILES
    ${modfiles}
    TARGET_LIBRARY_NAME
    ${usecase}_nocmodl
    EXTRA_ENV
    ${NRN_RUN_FROM_BUILD_DIR_ENV}
    NOCMODL_EXECUTABLE
    ${PROJECT_BINARY_DIR}/bin/nocmodl
    OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nocmodl/${CMAKE_HOST_SYSTEM_PROCESSOR}")
  add_dependencies(${usecase}_nocmodl neuron::nrniv neuron::nocmodl)
  target_include_directories(${usecase}_nocmodl PRIVATE "${PROJECT_BINARY_DIR}/include")
  set_target_properties(
    ${usecase}_nocmodl
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nocmodl/${CMAKE_HOST_SYSTEM_PROCESSOR}")

  # build modfiles with NMODL
  create_nrnmech(
    NEURON
    NMODL_NEURON_CODEGEN
    MOD_FILES
    ${modfiles}
    NMODL_EXTRA_ARGS
    codegen
    --cvode
    EXTRA_ENV
    ${NRN_RUN_FROM_BUILD_DIR_ENV}
    TARGET_LIBRARY_NAME
    ${usecase}_nmodl
    NMODL_EXECUTABLE
    ${PROJECT_BINARY_DIR}/bin/nmodl
    OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nmodl/${CMAKE_HOST_SYSTEM_PROCESSOR}")
  add_dependencies(${usecase}_nmodl nmodl::nmodl neuron::nrniv)
  target_include_directories(${usecase}_nmodl PRIVATE "${PROJECT_BINARY_DIR}/include")
  set_target_properties(
    ${usecase}_nmodl
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nmodl/${CMAKE_HOST_SYSTEM_PROCESSOR}")

  # add all usecase tests
  file(GLOB testfiles "${usecase}/test_*.py" "${usecase}/simulate.py")
  foreach(testfile ${testfiles})
    get_filename_component(abs_testfile "${testfile}" REALPATH)
    string(SHA256 hash "${testfile}")
    # run with NOCMODL beforehand because it may generate some files
    add_custom_target(
      usecase_nocmodl_${usecase}_${hash}
      COMMAND ${CMAKE_COMMAND} -E env "${NRN_RUN_FROM_BUILD_DIR_ENV}"
              ${NRN_DEFAULT_PYTHON_EXECUTABLE} "${abs_testfile}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nocmodl")

    add_test(
      NAME usecase_nmodl_${usecase}_${hash}
      COMMAND ${CMAKE_COMMAND} -E env "${NRN_RUN_FROM_BUILD_DIR_ENV}"
              ${NRN_DEFAULT_PYTHON_EXECUTABLE} "${abs_testfile}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nmodl")
    set_tests_properties(usecase_nmodl_${usecase}_${hash}
                         PROPERTIES DEPENDS usecase_nocmodl_${usecase}_${hash})
  endforeach()
endforeach()

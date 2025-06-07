include("${PROJECT_SOURCE_DIR}/cmake/NeuronTestHelper.cmake")

function(add_usecase_test usecase)
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
    ARTIFACTS_OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nocmodl/${CMAKE_HOST_SYSTEM_PROCESSOR}"
    LIBRARY_OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nocmodl/${CMAKE_HOST_SYSTEM_PROCESSOR}")
  add_dependencies(${usecase}_nocmodl neuron::nrniv neuron::nocmodl)
  target_include_directories(${usecase}_nocmodl PRIVATE "${PROJECT_BINARY_DIR}/include")

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
    ARTIFACTS_OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nmodl/${CMAKE_HOST_SYSTEM_PROCESSOR}"
    LIBRARY_OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nmodl/${CMAKE_HOST_SYSTEM_PROCESSOR}")
  add_dependencies(${usecase}_nmodl nmodl::nmodl neuron::nrniv)
  target_include_directories(${usecase}_nmodl PRIVATE "${PROJECT_BINARY_DIR}/include")

  # add all usecase tests
  file(GLOB testfiles "${usecase}/test_*.py" "${usecase}/simulate.py")
  foreach(testfile ${testfiles})
    get_filename_component(abs_testfile "${testfile}" REALPATH)
    get_filename_component(testname "${abs_testfile}" NAME_WLE)
    add_test(
      NAME nmodl::usecase::${usecase}::${testname}
      COMMAND ${CMAKE_COMMAND} -E env ${NRN_RUN_FROM_BUILD_DIR_ENV} ${NRN_DEFAULT_PYTHON_EXECUTABLE}
              "${abs_testfile}" nmodl "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nocmodl"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${usecase}/nmodl")
  endforeach()

endfunction()

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
  add_usecase_test(${usecase})
endforeach()

set(dependencies neuron::nrniv neuron::nocmodl)
set(outputs "${CMAKE_CURRENT_BINARY_DIR}/steady_state/nocmodl/test_minipump.pkl"
            "${CMAKE_CURRENT_BINARY_DIR}/steady_state/nocmodl/test_minipump-steady_state.pkl")

# the steady_state and longitudinal_diffusion tests require NOCMODL to actually generate some files
add_custom_command(
  OUTPUT ${outputs}
  COMMAND
    ${CMAKE_COMMAND} -E env ${NRN_RUN_FROM_BUILD_DIR_ENV} ${NRN_DEFAULT_PYTHON_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/steady_state/test_minipump.py" nocmodl
    "${CMAKE_CURRENT_BINARY_DIR}/steady_state/nocmodl"
  DEPENDS ${dependencies}
  VERBATIM
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/steady_state/nocmodl")

add_custom_target(usecase_steady_state ALL DEPENDS ${outputs})

add_dependencies(usecase_steady_state steady_state_nocmodl)

file(GLOB modfiles "longitudinal_diffusion/*.mod")
set(outputs)
foreach(modfile "${modfiles}")
  get_filename_component(filename "${modfile}" NAME_WLE)
  list(APPEND outputs
       "${CMAKE_CURRENT_BINARY_DIR}/longitudinal_diffusion/nocmodl/diffuse-${filename}.pkl")
endforeach()
add_custom_command(
  OUTPUT ${outputs}
  COMMAND
    ${CMAKE_COMMAND} -E env ${NRN_RUN_FROM_BUILD_DIR_ENV} ${NRN_DEFAULT_PYTHON_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/longitudinal_diffusion/test_heat_eqn.py" nocmodl
    "${CMAKE_CURRENT_BINARY_DIR}/longitudinal_diffusion/nocmodl"
  DEPENDS ${dependencies}
  VERBATIM
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/longitudinal_diffusion/nocmodl")

add_custom_target(usecase_longitudinal_diffusion ALL DEPENDS ${outputs})

add_dependencies(usecase_longitudinal_diffusion longitudinal_diffusion_nocmodl)

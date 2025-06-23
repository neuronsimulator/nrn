include("${PROJECT_SOURCE_DIR}/cmake/NeuronMechVariableHelper.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/neuronMechMaker.cmake")
set(SOURCE_FILES
    cabpump.mod
    cachan1.mod
    camchan.mod
    capmpr.mod
    capump.mod
    invlfire.mod
    khhchan.mod
    nacaex.mod
    nachan.mod
    release.mod)
create_nrnmech(
  NEURON
  SPECIAL
  MOD_FILES
  ${SOURCE_FILES}
  EXTRA_ENV
  ASAN_OPTIONS=detect_leaks=0
  TARGET_LIBRARY_NAME
  neurondemo_lib
  TARGET_EXECUTABLE_NAME
  neurondemo_bin
  # TODO it would probably make more sense to use CMAKE_CURRENT_BINARY_DIR here, so either move this
  # dir to `share/nrn/demo`, or modify scripts to handle `share/demo` instead
  EXECUTABLE_OUTPUT_DIR
  "${PROJECT_BINARY_DIR}/share/nrn/demo/release/${CMAKE_HOST_SYSTEM_PROCESSOR}"
  LIBRARY_OUTPUT_DIR
  "${PROJECT_BINARY_DIR}/share/nrn/demo/release/${CMAKE_HOST_SYSTEM_PROCESSOR}")
target_include_directories(neurondemo_lib PUBLIC "${PROJECT_BINARY_DIR}/include")
# because nrniv is under `<prefix>/lib`, but the demo files are in
# `<prefix>/share/nrn/demo/release/<arch>/`
set_target_properties(
  neurondemo_bin neurondemo_lib
  PROPERTIES INSTALL_RPATH "${LOADER_PATH_FLAG};${LOADER_PATH_FLAG}/../../../../../lib")

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/share/nrn/demo")
file(TOUCH "${PROJECT_BINARY_DIR}/share/nrn/demo/neuron")
# also install all of the source files (except current CMakeLists.txt)
install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  DESTINATION "${NRN_INSTALL_SHARE_DIR}"
  PATTERN "CMakeLists.txt" EXCLUDE)
install(TARGETS neurondemo_lib neurondemo_bin
        DESTINATION "${NRN_INSTALL_SHARE_DIR}/demo/release/${CMAKE_HOST_SYSTEM_PROCESSOR}")

include(${PROJECT_SOURCE_DIR}/cmake/NeuronMechVariableHelper.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/neuronMechMaker.cmake)

# compile all mod files and install them
file(GLOB neurondemo_files "${CMAKE_CURRENT_SOURCE_DIR}/release/*.mod")
create_nrnmech(
  SPECIAL
  MOD_FILES
  ${neurondemo_files}
  TARGET_LIBRARY_NAME
  neurondemo
  TARGET_EXECUTABLE_NAME
  neurondemo-special
  NOCMODL_EXECUTABLE
  ${PROJECT_BINARY_DIR}/bin/nocmodl
  EXTRA_ENV
  ${NRN_RUN_FROM_BUILD_DIR_ENV}
  OUTPUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}/release")
target_include_directories(neurondemo PRIVATE "${PROJECT_BINARY_DIR}/include")
target_include_directories(neurondemo-special PRIVATE "${PROJECT_BINARY_DIR}/include")
add_dependencies(neurondemo neuron::nocmodl neuron::nrniv)
add_dependencies(neurondemo-special neuron::nocmodl neuron::nrniv)
install(TARGETS neurondemo
        DESTINATION "${NRN_INSTALL_SHARE_DIR}/demo/release/${CMAKE_HOST_SYSTEM_PROCESSOR}/.libs")
install(TARGETS neurondemo-special
        DESTINATION "${NRN_INSTALL_SHARE_DIR}/demo/release/${CMAKE_HOST_SYSTEM_PROCESSOR}/")
# also install all of the source files (except current CMakeLists.txt)
install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  DESTINATION "${NRN_INSTALL_SHARE_DIR}"
  PATTERN "CMakeLists.txt" EXCLUDE)

# Traub 1/10 (ModelDB 82894) — product native-GPU spike gate.
#
# Model stays outside the tree ($HOME/models/82894 or NRN_TRAUB_MODEL).
# Tools + reference rasters live here. CTest skips (exit 77) if the model
# is not present so GPU CI without ModelDB remains green.
#
#   ctest -V -R 'traub_native::neuron_gpu_native'
#   ctest -V -R 'traub_native::neuron_gpu_native_gap'   # optional / longer
if(NOT NRN_ENABLE_GPU)
  return()
endif()

set(traub_harness_src "${CMAKE_CURRENT_SOURCE_DIR}/run_traub_native.sh")
set(traub_harness_build "${PROJECT_BINARY_DIR}/test/traub_native/run_traub_native.sh")
set(traub_mech_dir "${PROJECT_BINARY_DIR}/test/traub_native/mechs")
set(traub_tmpdir "${PROJECT_BINARY_DIR}/test/tmp/traub_native")
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/test/traub_native")
file(MAKE_DIRECTORY "${traub_mech_dir}")
file(MAKE_DIRECTORY "${traub_tmpdir}")

# Pure bash; invoke as `bash …` so +x is not required.
configure_file("${traub_harness_src}" "${traub_harness_build}" COPYONLY)

# NRN_RUN_FROM_BUILD_DIR_ENV is a CMake list of KEY=VAL entries (PATH, LD_*, …).
set(traub_env ${NRN_RUN_FROM_BUILD_DIR_ENV})
list(
  APPEND
  traub_env
  "OMP_NUM_THREADS=1"
  "NRN_GPU_BACKEND_TEST=native"
  "NRN_GPU_PERMUTE=2"
  "NRN_TRAUB_SKIP_IF_NO_MODEL=1"
  "NRN_TRAUB_MECH_DIR=${traub_mech_dir}"
  "NRN_TRAUB_REF_DIR=${CMAKE_CURRENT_SOURCE_DIR}/reference"
  "TMPDIR=${traub_tmpdir}")
if(DEFINED ENV{NRN_TRAUB_MODEL} AND NOT "$ENV{NRN_TRAUB_MODEL}" STREQUAL "")
  list(APPEND traub_env "NRN_TRAUB_MODEL=$ENV{NRN_TRAUB_MODEL}")
endif()

# Serial no-gap product gate (~10–20 s after mechs built; first nrnivmodl longer).
add_test(
  NAME traub_native::neuron_gpu_native
  COMMAND ${CMAKE_COMMAND} -E env ${traub_env} bash "${traub_harness_build}" --no-gap
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/test/traub_native")
set_tests_properties(
  traub_native::neuron_gpu_native
  PROPERTIES RESOURCE_LOCK gpu SKIP_RETURN_CODE 77 TIMEOUT 900 LABELS "gpu;native;traub")

# Optional gap gate (exact native↔CPU 7873; longer wall). Same skip policy.
add_test(
  NAME traub_native::neuron_gpu_native_gap
  COMMAND ${CMAKE_COMMAND} -E env ${traub_env} bash "${traub_harness_build}" --gap
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/test/traub_native")
set_tests_properties(
  traub_native::neuron_gpu_native_gap
  PROPERTIES RESOURCE_LOCK gpu SKIP_RETURN_CODE 77 TIMEOUT 1200 LABELS "gpu;native;traub;gap")

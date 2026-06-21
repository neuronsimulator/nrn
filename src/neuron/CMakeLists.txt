# =============================================================================
# NEURON 9.0 model-data layer (src/neuron/) — GPU offload scaffold and future native GPU engine
# sources.
# =============================================================================
add_library(neuron_gpu STATIC gpu/offload.cpp gpu/device_state.cpp gpu/config.cpp gpu/device_assign.cpp)
set_property(TARGET neuron_gpu PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(neuron_gpu PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(neuron_gpu PRIVATE fmt::fmt)

cpp_cc_configure_sanitizers(TARGET neuron_gpu)
if(NRN_ENABLE_GPU)
  target_compile_definitions(neuron_gpu PUBLIC NRN_ENABLE_GPU)
endif()
if(NRN_ENABLE_GPU AND CORENRN_ENABLE_GPU)
  separate_arguments(_neuron_gpu_acc_flags UNIX_COMMAND "${CORENRN_ACC_COMP_FLAGS}")
  set(_neuron_gpu_openacc_sources gpu/offload.cpp)
  set_source_files_properties(
    ${_neuron_gpu_openacc_sources}
    PROPERTIES COMPILE_OPTIONS "${_neuron_gpu_acc_flags}" COMPILE_DEFINITIONS
                                                          "CORENEURON_ENABLE_GPU;NRN_ENABLE_GPU")

  # upload.cpp pulls NEURON/oc headers; compile as OpenACC object lib for libnrniv + GPU tests.
  add_library(neuron_gpu_upload OBJECT gpu/upload.cpp gpu/fadvance_gpu.cpp gpu/net_events.cpp
                                         gpu/net_send_buffer.cpp gpu/sync.cpp)
  set_property(TARGET neuron_gpu_upload PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_include_directories(
    neuron_gpu_upload
    PRIVATE ${PROJECT_SOURCE_DIR}/src
            ${PROJECT_SOURCE_DIR}/src/nrniv
            ${PROJECT_SOURCE_DIR}/src/nrnoc
            ${PROJECT_SOURCE_DIR}/src/oc
            ${PROJECT_BINARY_DIR}
            ${PROJECT_BINARY_DIR}/src/nrnoc
            ${PROJECT_BINARY_DIR}/src/oc
            ${PROJECT_SOURCE_DIR}/external/fmt/include)
  target_link_libraries(neuron_gpu_upload PRIVATE fmt::fmt)
  set_source_files_properties(
    gpu/upload.cpp gpu/fadvance_gpu.cpp gpu/net_events.cpp gpu/net_send_buffer.cpp gpu/sync.cpp
    PROPERTIES COMPILE_OPTIONS "${_neuron_gpu_acc_flags}" COMPILE_DEFINITIONS
                                                          "CORENEURON_ENABLE_GPU;NRN_ENABLE_GPU")
endif()

# =============================================================================
# NEURON 9.0 model-data layer (src/neuron/) — GPU offload scaffold and future native GPU engine
# sources.
# =============================================================================
add_library(neuron_gpu STATIC gpu/offload.cpp gpu/device_state.cpp)
set_property(TARGET neuron_gpu PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(neuron_gpu PUBLIC ${PROJECT_SOURCE_DIR}/src)
cpp_cc_configure_sanitizers(TARGET neuron_gpu)
if(NRN_ENABLE_GPU)
  target_compile_definitions(neuron_gpu PUBLIC NRN_ENABLE_GPU)
endif()

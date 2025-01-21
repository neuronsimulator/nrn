if(NMODL_ENABLE_PYTHON_BINDINGS)
  install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/nmodl"
    DESTINATION lib/python/neuron
    PATTERN "__pycache__" EXCLUDE)
endif()

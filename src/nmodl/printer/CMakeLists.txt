# =============================================================================
# Printer library
# =============================================================================
add_library(printer OBJECT code_printer.cpp json_printer.cpp nmodl_printer.cpp)
set_property(TARGET printer PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(printer PRIVATE util)

if(NMODL_ENABLE_BACKWARD)
  target_link_libraries(printer PRIVATE Backward::Interface)
  target_compile_definitions(printer PUBLIC NMODL_ENABLE_BACKWARD=1)
endif()

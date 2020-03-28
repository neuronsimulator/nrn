# =============================================================================
# Printer sources
# =============================================================================
set(PRINTER_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/json_printer.hpp ${CMAKE_CURRENT_SOURCE_DIR}/json_printer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_printer.hpp ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_printer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code_printer.hpp ${CMAKE_CURRENT_SOURCE_DIR}/code_printer.cpp)

# =============================================================================
# Printer library
# =============================================================================

add_library(printer_obj OBJECT ${PRINTER_SOURCE_FILES})
set_property(TARGET printer_obj PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(printer STATIC $<TARGET_OBJECTS:printer_obj>)

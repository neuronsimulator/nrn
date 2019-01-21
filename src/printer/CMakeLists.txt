#=============================================================================
# Printer sources
#=============================================================================
set(PRINTER_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/json_printer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/json_printer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_printer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_printer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code_printer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code_printer.cpp
)

#=============================================================================
# Printer library
#=============================================================================
add_library(printer
            STATIC
            ${PRINTER_SOURCE_FILES})
set_property(TARGET printer PROPERTY POSITION_INDEPENDENT_CODE ON)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${PRINTER_SOURCE_FILES}
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)
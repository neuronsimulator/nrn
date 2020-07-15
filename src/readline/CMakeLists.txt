# =============================================================================
# NEURON distributed readline library (specifically for OSX)
# =============================================================================
set(READLINE_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/funmap.c ${CMAKE_CURRENT_SOURCE_DIR}/history.c
    ${CMAKE_CURRENT_SOURCE_DIR}/keymaps.c ${CMAKE_CURRENT_SOURCE_DIR}/readline.c)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# =============================================================================
# readline library
# =============================================================================
add_definitions("-w")
add_library(readline STATIC ${READLINE_SOURCE_FILES})
set_property(TARGET readline PROPERTY POSITION_INDEPENDENT_CODE ON)

# =============================================================================
# Install target
# =============================================================================
install(TARGETS readline DESTINATION lib)

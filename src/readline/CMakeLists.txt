# =============================================================================
# NEURON distributed readline library (specifically for OSX)
# =============================================================================
set(READLINE_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/funmap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/history.c
    ${CMAKE_CURRENT_SOURCE_DIR}/keymaps.c
    ${CMAKE_CURRENT_SOURCE_DIR}/readline.c)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(curses REQUIRED)

# =============================================================================
# readline library
# =============================================================================
add_library(readline SHARED ${READLINE_SOURCE_FILES})
target_link_libraries(readline curses)

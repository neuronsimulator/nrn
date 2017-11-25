cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(nocmodl CXX)

#=============================================================================
# CMake common project settings
#=============================================================================
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#=============================================================================
# Find required packages
#=============================================================================
message(STATUS "CHECKING FOR FLEX/BISON/PYTHON")

find_package(PythonInterp REQUIRED)
find_package(FLEX 2.6 REQUIRED)
find_package(BISON 3.0 REQUIRED)

#=============================================================================
# Include cmake modules
#=============================================================================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(GitRevision)
include(FlexHelper)
include(CompilerHelper)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/ext/
)

#=============================================================================
# Project version from git and project directories
#=============================================================================
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# generate file with version number from git
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version/version.cpp.in
               ${CMAKE_CURRENT_BINARY_DIR}/version.cpp @ONLY)

add_subdirectory(src/language)
add_subdirectory(src/lexer)
add_subdirectory(src/parser)
add_subdirectory(src/visitors)

#=============================================================================
# Memory checker options and add tests
#=============================================================================
find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes \
                                 --leak-check=full \
                                 --track-origins=yes \
                                 --show-possibly-lost=no")
include (CTest)
add_subdirectory(test)

#=============================================================================
# Add executables
#=============================================================================
add_executable(nocmodl
               src/main.cpp
               ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

target_link_libraries(nocmodl lexer visitor)

#=============================================================================
# Clang format options
#=============================================================================
add_custom_target(clangformat
                  COMMAND
                  clang-format -i
                  --style=file ${FILES_FOR_CLANG_FORMAT})

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(nocmodl CXX)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "CHECKING FOR FLEX/BISON/PYTHON")

find_package(PythonInterp REQUIRED)
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(GitRevision)

include_directories(${PROJECT_SOURCE_DIR}/src)

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# generate source file with version number
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version/version.cpp.in
               ${CMAKE_CURRENT_BINARY_DIR}/version.cpp @ONLY)

ADD_EXECUTABLE(${PROJECT_NAME}
               src/main.cpp
               ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

add_custom_target(clangformat
                  COMMAND
                  clang-format -i
                  --style=file ${FILES_FOR_CLANG_FORMAT})

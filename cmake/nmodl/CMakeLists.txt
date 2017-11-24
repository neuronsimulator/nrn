cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(nocmodl CXX)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

message(STATUS "CHECKING FOR FLEX/BISON/PYTHON")

find_package(PythonInterp REQUIRED)
find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(GitRevision)
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/ext/
)

# Important : make sure to have flex header included
# at top level. Otherwise get strange compilation and
# runtime memory errors due to inclusion of older version
# from /usr/include (which is always present!)
include_directories(${FLEX_INCLUDE_DIRS})

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# generate file with version number from git
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version/version.cpp.in
               ${CMAKE_CURRENT_BINARY_DIR}/version.cpp @ONLY)

add_subdirectory(src/language)
add_subdirectory(src/lexer)
add_subdirectory(src/parser)
add_subdirectory(src/visitors)
add_subdirectory(test)

include (CTest)
add_test (NAME ModToken COMMAND testmodtoken)
add_test (NAME Lexer COMMAND testlexer)

add_executable(nocmodl
               src/main.cpp
               ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

target_link_libraries(nocmodl lexer visitor)

add_custom_target(clangformat
                  COMMAND
                  clang-format -i
                  --style=file ${FILES_FOR_CLANG_FORMAT})
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
# This is called from PythonHelper.cmake in a subprocess, to allow multiple Python versions to be
# searched for in the project without CACHE variable hackery.
project(ExecuteFindPython LANGUAGES C)
find_package(Python3 COMPONENTS ${Python3_COMPONENTS})
message(STATUS "Python3_INCLUDE_DIRS=${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_LIBRARIES=${Python3_LIBRARIES}")
message(STATUS "Python3_VERSION_MAJOR=${Python3_VERSION_MAJOR}")
message(STATUS "Python3_VERSION_MINOR=${Python3_VERSION_MINOR}")

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
# This is called from PythonHelper.cmake in a subprocess, to allow multiple Python versions to be
# searched for in the project without CACHE variable hackery.
project(ExecuteFindPython LANGUAGES C)
find_package(Python COMPONENTS ${Python_COMPONENTS})
message(STATUS "Python_INCLUDE_DIRS=${Python_INCLUDE_DIRS}")
message(STATUS "Python_LIBRARIES=${Python_LIBRARIES}")
# DLL beside the import library on Windows. NOTFOUND on Unix, where Python_LIBRARIES is the shared
# object. FindNmodlPyLib dlopens this instead of a .lib or .dll.a.
message(STATUS "Python_RUNTIME_LIBRARY=${Python_RUNTIME_LIBRARY}")
message(STATUS "Python_VERSION=${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")

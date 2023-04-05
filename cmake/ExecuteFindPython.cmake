# This is called from PythonHelper.cmake in a subprocess, to allow multiple Python versions to be
# searched for in the project without CACHE variable hackery.
find_package(Python3 COMPONENTS Development Interpreter)
message(STATUS "Python3_INCLUDE_DIRS=${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_LIBRARIES=${Python3_LIBRARIES}")
message(STATUS "Python3_VERSION_MAJOR=${Python3_VERSION_MAJOR}")
message(STATUS "Python3_VERSION_MINOR=${Python3_VERSION_MINOR}")

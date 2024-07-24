#.rst:
#
# Find ``cython`` executable.
#
# This module will set the following variables in your project:
#
#  ``CYTHON_EXECUTABLE``
#    path to the ``cython`` program
#
#  ``CYTHON_VERSION``
#    version of ``cython``
#
#  ``CYTHON_FOUND``
#    true if the program was found
#
# And the following target:
#
#  ``Cython::Cython``
#    The Cython executable
#
# A range of versions is supported on CMake 3.19+. See also UseCython.
#
# For more information on the Cython project, see https://cython.org/.
#
# *Cython is a language that makes writing C extensions for the Python language
# as easy as Python itself.*
#
#=============================================================================
# Copyright 2011 Kitware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#=============================================================================

# Use the Cython executable that lives next to the Python executable
# if it is a local installation.
if(Python_EXECUTABLE)
  get_filename_component(_python_path ${Python_EXECUTABLE} PATH)
elseif(Python3_EXECUTABLE)
  get_filename_component(_python_path ${Python3_EXECUTABLE} PATH)
elseif(DEFINED PYTHON_EXECUTABLE)
  get_filename_component(_python_path ${PYTHON_EXECUTABLE} PATH)
endif()

if(DEFINED _python_path)
  find_program(CYTHON_EXECUTABLE
               NAMES cython cython.bat cython3
               HINTS ${_python_path}
               DOC "path to the cython executable")
else()
  find_program(CYTHON_EXECUTABLE
               NAMES cython cython.bat cython3
               DOC "path to the cython executable")
endif()

if(CYTHON_EXECUTABLE)
  set(CYTHON_version_command "${CYTHON_EXECUTABLE}" --version)

  execute_process(COMMAND ${CYTHON_version_command}
                  OUTPUT_VARIABLE CYTHON_version_output
                  ERROR_VARIABLE CYTHON_version_error
                  RESULT_VARIABLE CYTHON_version_result
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_STRIP_TRAILING_WHITESPACE)

  if(NOT ${CYTHON_version_result} EQUAL 0)
    set(_error_msg "Command \"${CYTHON_version_command}\" failed with")
    set(_error_msg "${_error_msg} output:\n${CYTHON_version_error}")
    message(FATAL_ERROR "${_error_msg}")
  elseif("${CYTHON_version_output}" MATCHES "^[Cc]ython version ([^,]+)")
    set(CYTHON_VERSION "${CMAKE_MATCH_1}")
  elseif("${CYTHON_version_error}" MATCHES "^[Cc]ython version ([^,]+)")
    set(CYTHON_VERSION "${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR "Invalid Cython version output")
  endif()
endif()

include(FindPackageHandleStandardArgs)

if(CMAKE_VERSION VERSION_LESS 3.19)
  set(_handle_version_range)
else()
  set(_handle_version_range HANDLE_VERSION_RANGE)
endif()

find_package_handle_standard_args(Cython
  REQUIRED_VARS CYTHON_EXECUTABLE
  VERSION_VAR ${CYTHON_VERSION}
  ${_handle_version_range}
)

if(CYTHON_FOUND)
  if(NOT DEFINED Cython::Cython)
    add_executable(Cython::Cython IMPORTED)
    set_target_properties(Cython::Cython PROPERTIES
        IMPORTED_LOCATION "${CYTHON_EXECUTABLE}"
    )
  endif()
endif()

mark_as_advanced(CYTHON_EXECUTABLE)

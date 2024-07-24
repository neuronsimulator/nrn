#.rst:
#
# The following functions are defined:
#
# .. cmake:command:: Cython_transpile
#
# Create custom rules to generate the source code for a Python extension module
# using cython.
#
#   Cython_transpile(<pyx_file>
#                   [LANGUAGE C | CXX]
#                   [CYTHON_ARGS <args> ...]
#                   [OUTPUT <OutputFile>]
#                   [OUTPUT_VARIABLE <OutputVariable>])
#
# Options:
#
# ``LANGUAGE [C | CXX]``
#   Force the generation of either a C or C++ file. Recommended; will attempt
#   to be deduced if not specified, defaults to C unless only CXX is enabled.
#
# ``CYTHON_ARGS <args>``
#   Specify additional arguments for the cythonization process. Will default to
#   the ``CYTHON_ARGS`` variable if not specified.
#
# ``OUTPUT <OutputFile>``
#   Specify a specific path for the output file as ``<OutputFile>``. By
#   default, this will output into the current binary dir. A depfile will be
#   created alongside this file as well.
#
# ``OUTPUT_VARIABLE <OutputVariable>``
#   Set the variable ``<OutputVariable>`` in the parent scope to the path to the
#   generated source file.
#
# Defined variables:
#
# ``<OutputVariable>``
#   The path of the generated source file.
#
# Usage example:
#
# .. code-block:: cmake
#
#   find_package(Cython)
#   include(UseCython)
#
#   Cython_transpile(_hello.pyx
#     OUTPUT_VARIABLE _hello_source_files
#   )
#
#   Python_add_library(_hello
#     MODULE ${_hello_source_files}
#     WITH_SOABI
#   )
#
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

if(CMAKE_VERSION VERSION_LESS "3.8")
  # CMake 3.7 required for DEPFILE
  # CMake 3.8 required for COMMAND_EXPAND_LISTS
  message(FATAL_ERROR "CMake 3.8 required for COMMAND_EXPAND_LISTS")
endif()

function(Cython_transpile)
  set(_options )
  set(_one_value LANGUAGE OUTPUT OUTPUT_VARIABLE)
  set(_multi_value CYTHON_ARGS)

  cmake_parse_arguments(_args
    "${_options}"
    "${_one_value}"
    "${_multi_value}"
    ${ARGN}
    )

  if(DEFINED CYTHON_EXECUTABLE)
    set(_cython_command "${CYTHON_EXECUTABLE}")
  elseif(DEFINED Python_EXECUTABLE)
    set(_cython_command "${Python_EXECUTABLE}" -m cython)
  elseif(DEFINED Python3_EXECUTABLE)
    set(_cython_command "${Python3_EXECUTABLE}" -m cython)
  else()
    message(FATAL_ERROR "Cython executable not found")
  endif()

  # Default to CYTHON_ARGS if argument not specified
  if(NOT _args_CYTHON_ARGS AND DEFINED CYTHON_ARGS)
    set(_args_CYTHON_ARGS "${CYTHON_ARGS}")
  endif()

  # Get input
  set(_source_files ${_args_UNPARSED_ARGUMENTS})
  list(LENGTH _source_files input_length)
  if(NOT input_length EQUAL 1)
    message(FATAL_ERROR "One and only one input file must be specified, got '${_source_files}'")
  endif()

  function(_transpile _source_file generated_file language)

    if(language STREQUAL "C")
      set(_language_arg "")
    elseif(language STREQUAL "CXX")
      set(_language_arg "--cplus")
    else()
      message(FATAL_ERROR "_transpile language must be one of C or CXX")
    endif()

    set_source_files_properties(${generated_file} PROPERTIES GENERATED TRUE)

    # Generated depfile is expected to have the ".dep" extension and be located along
    # side the generated source file.
    set(_depfile ${generated_file}.dep)
    set(_depfile_arg "-M")

    # Normalize the input path
    get_filename_component(_source_file "${_source_file}" ABSOLUTE)

    # Pretty-printed output names
    file(RELATIVE_PATH generated_file_relative
        ${CMAKE_BINARY_DIR} ${generated_file})
    file(RELATIVE_PATH source_file_relative
        ${CMAKE_SOURCE_DIR} ${_source_file})
    set(comment "Generating ${_language} source '${generated_file_relative}' from '${source_file_relative}'")

    # Get output directory to ensure its exists
    get_filename_component(output_directory "${generated_file}" DIRECTORY)

    get_source_file_property(pyx_location ${_source_file} LOCATION)

    # Add the command to run the compiler.
    add_custom_command(
      OUTPUT ${generated_file}
      COMMAND
        ${CMAKE_COMMAND} -E make_directory ${output_directory}
      COMMAND
        ${_cython_command}
        ${_language_arg}
        "${_args_CYTHON_ARGS}"
        ${_depfile_arg}
        ${pyx_location}
        --output-file ${generated_file}
      COMMAND_EXPAND_LISTS
      MAIN_DEPENDENCY
        ${_source_file}
      DEPFILE
        ${_depfile}
      VERBATIM
      COMMENT ${comment}
    )
  endfunction()

  function(_set_output _input_file _language _output_var)
    if(_language STREQUAL "C")
      set(_language_extension "c")
    elseif(_language STREQUAL "CXX")
      set(_language_extension "cxx")
    else()
      message(FATAL_ERROR "_set_output language must be one of C or CXX")
    endif()

    # Can use cmake_path for CMake 3.20+
    # cmake_path(GET _input_file STEM basename)
    get_filename_component(_basename "${_input_file}" NAME_WE)

    if(IS_ABSOLUTE ${_input_file})
      file(RELATIVE_PATH _input_relative ${CMAKE_CURRENT_SOURCE_DIR} ${_input_file})
    else()
      set(_input_relative ${_input_file})
    endif()

    get_filename_component(_output_relative_dir "${_input_relative}" DIRECTORY)
    string(REPLACE "." "_" _output_relative_dir "${_output_relative_dir}")
    if(_output_relative_dir)
      set(_output_relative_dir "${_output_relative_dir}/")
    endif()

    set(${_output_var} "${CMAKE_CURRENT_BINARY_DIR}/${_output_relative_dir}${_basename}.${_language_extension}" PARENT_SCOPE)
  endfunction()

  set(generated_files)

  list(GET _source_files 0 _source_file)

  # Set target language
  set(_language ${_args_LANGUAGE})
  if(NOT _language)
    get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    if("C" IN_LIST _languages AND "CXX" IN_LIST _languages)
      # Try to compute language. Returns falsy if not found.
      _cython_compute_language(_language ${_source_file})
    elseif("C" IN_LIST _languages)
      # If only C is enabled globally, assume C
      set(_language "C")
    elseif("CXX" IN_LIST _languages)
      # Likewise for CXX
      set(_language "CXX")
    else()
      message(FATAL_ERROR "LANGUAGE keyword required if neither C nor CXX enabled globally")
    endif()
  endif()

  if(NOT _language MATCHES "^(C|CXX)$")
    message(FATAL_ERROR "Cython_transpile LANGUAGE must be one of C or CXX")
  endif()

  # Place the cython files in the current binary dir if no path given
  if(NOT _args_OUTPUT)
    _set_output(${_source_file} ${_language} _args_OUTPUT)
  elseif(NOT IS_ABSOLUTE ${_args_OUTPUT})
    set(_args_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_args_OUTPUT}")
  endif()

  set(generated_file ${_args_OUTPUT})
  _transpile(${_source_file} ${generated_file} ${_language})
  list(APPEND generated_files ${generated_file})

  # Output variable only if set
  if(_args_OUTPUT_VARIABLE)
    set(_output_variable ${_args_OUTPUT_VARIABLE})
    set(${_output_variable} ${generated_files} PARENT_SCOPE)
  endif()

endfunction()

function(_cython_compute_language OUTPUT_VARIABLE FILENAME)
  file(READ "${FILENAME}" FILE_CONTENT)
  # Check for compiler directive similar to "# distutils: language = c++"
  # See https://cython.readthedocs.io/en/latest/src/userguide/wrapping_CPlusPlus.html#declare-a-var-with-the-wrapped-c-class
  set(REGEX_PATTERN [=[^[[:space:]]*#[[:space:]]*distutils:.*language[[:space:]]*=[[:space:]]*(c\\+\\+|c)]=])
  string(REGEX MATCH "${REGEX_PATTERN}" MATCH_RESULT "${FILE_CONTENT}")
  string(TOUPPER "${MATCH_RESULT}" LANGUAGE_NAME)
  string(REPLACE "+" "X" LANGUAGE_NAME "${LANGUAGE_NAME}")
  set(${OUTPUT_VARIABLE} ${LANGUAGE_NAME} PARENT_SCOPE)
endfunction()

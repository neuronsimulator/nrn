# ~~~
# Helper functions for compiling NEURON Python modules.
# Currently used for the following Python modules:
# - neuron.hoc
# - neuron.rxd.geometry3d.surfaces
# - neuron.rxd.geometry3d.ctng
# - neuron.rxd.geometry3d.graphicsPrimitives
# - neuronmusic
# ~~~

# ~~~
# cythonize(input_file
# [OUTPUT path/to/output]
# [LANGUAGE language]
# [PYTHON_EXECUTABLE path/to/executable]
# )
#
# Convert a pyx file into a c or cpp file using Cython.
#
# input_file        - the full path to the input file that should be converted
# OUTPUT            - the full path to the output file (with extension!)
# LANGUAGE          - the language used for the output file
# PYTHON_EXECUTABLE - (optional) the full path to the Python executable used for launching
#                     Cython. If not specified, defaults to the value of `CYTHON_EXECUTABLE`.
#
# Note that `find_package(Cython)` must be called before invoking this function!
# ~~~
function(cythonize input_file)
  cmake_parse_arguments(ARG "" "LANGUAGE;PYTHON_EXECUTABLE;OUTPUT" "" ${ARGN})
  string(TOUPPER "${ARG_LANGUAGE}" ARG_LANGUAGE)
  set(supported_languages "C" "CXX")
  if(NOT ARG_LANGUAGE IN_LIST supported_languages)
    message(
      FATAL_ERROR
        "Unsupported Cython output language: ${ARG_LANGUAGE}; supported languages are ${supported_languages}"
    )
  endif()

  if(NOT ARG_PYTHON_EXECUTABLE)
    set(ARG_PYTHON_EXECUTABLE ${CYTHON_EXECUTABLE})
  endif()

  if(ARG_LANGUAGE STREQUAL "CXX")
    set(command ${ARG_PYTHON_EXECUTABLE} -m cython --cplus)
  elseif(ARG_LANGUAGE STREQUAL "C")
    set(command ${ARG_PYTHON_EXECUTABLE} -m cython)
  endif()
  add_custom_command(
    OUTPUT ${ARG_OUTPUT}
    COMMAND ${command} ${input_file} --output-file ${ARG_OUTPUT}
    DEPENDS ${input_file}
    COMMENT "Cythonizing ${input_file} to ${ARG_OUTPUT}"
    VERBATIM)
endfunction()

# ~~~
# add_nrn_python_library(
# <name>
# [NO_EXTENSION]
# [HAS_FREE_THREADING]
# [TARGET target_name]
# [PYTHON_VERSION python_version]
# [LANGUAGE link_language]
# [OUTPUT_DIR path/to/output]
# [SOURCES src1 src2 ...]
# [INCLUDES dir1 dir2 ...]
# [LIBRARIES lib1 lib2 ...]
# [INSTALL_REL_RPATH path1 path2 ...])
#
# Create a target that links to nrnpython.
#
# <name>             - the actual name of the library being built
# NO_EXTENSION       - (optional, default unset) in case one wants to create a
#                      library without any platform-specific naming (so `hoc.so` instead of
#                      `hoc.cpython39-darwin.so` or similar). Note that no prefix is added.
# HAS_FREE_THREADING - (optional, default unset) whether the build of Python is free threaded
# TARGET             - (optional, defaults to <name>) the name of the CMake
#                      target. Can be anything, but may not conflict with existing targets.
# PYTHON_VERSION     - the version of Python to create the library for (for example, 3.9).
# LANGUAGE           - the language used for linking the library. See also the LINKER_LANGUAGE CMake variable.
# OUTPUT_DIR         - the path to the directory where the library will be placed afer building.
# SOURCES            - the list of source files used for compiling the library.
# INCLUDES           - (optional) the list of include directories.
# LIBRARIES          - (optional) the list of libraries we are linking with.
# INSTALL_REL_RPATH  - (optional) the list of RPATHs to use when installing the target.
# BUILD_REL_RPATH    - (optional) the list of RPATHs to use when building the target.
# ~~~
function(add_nrn_python_library name)
  set(options NO_EXTENSION HAS_FREE_THREADING)
  set(oneValueArgs TARGET PYTHON_VERSION LANGUAGE OUTPUT_DIR)
  set(multiValueArgs SOURCES INCLUDES LIBRARIES INSTALL_REL_RPATH BUILD_REL_RPATH)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_TARGET)
    set(ARG_TARGET "${name}")
  endif()

  add_library(${ARG_TARGET} MODULE ${ARG_SOURCES})
  if(ARG_INCLUDES)
    target_include_directories(${ARG_TARGET} PRIVATE ${ARG_INCLUDES})
  endif()
  if(ARG_LIBRARIES)
    target_link_libraries(${ARG_TARGET} PRIVATE ${ARG_LIBRARIES})
  endif()

  # Check passed version is supported
  if(NOT ${ARG_PYTHON_VERSION} IN_LIST NRN_PYTHON_VERSIONS)
    message(
      FATAL_ERROR
        "Unsupported Python version: ${ARG_PYTHON_VERSION} (available: ${NRN_PYTHON_VERSIONS})")
  endif()

  # Iterate over Python versions and find the right Python library (needed for Windows)
  foreach(val RANGE ${NRN_PYTHON_ITERATION_LIMIT})
    list(GET NRN_PYTHON_LIBRARIES ${val} nrnlib)
    list(GET NRN_PYTHON_VERSIONS ${val} nrnver)
    if("${nrnver}" STREQUAL "${ARG_PYTHON_VERSION}")
      break()
    endif()
  endforeach()

  # set platform-specific config
  if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    set(rel_rpath_name "@loader_path")
    set(os_string "darwin")
    set(lib_suffix "${CMAKE_SHARED_MODULE_SUFFIX}")
    set(python_interp "cpython-")
    set(undefined_link_flag "-Wl,-undefined,dynamic_lookup")
  elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(rel_rpath_name "$ORIGIN")
    # sometimes CMAKE_LIBRARY_ARCHITECTURE is not set, so here we build it manually
    if(NOT CMAKE_LIBRARY_ARCHITECTURE)
      set(arch "${CMAKE_SYSTEM_PROCESSOR}")
      string(TOLOWER "${CMAKE_SYSTEM_NAME}" os)

      if(arch STREQUAL "x86_64" OR arch STREQUAL "aarch64")
        set(lib_arch "${arch}-linux-gnu")
      else()
        set(lib_arch "${arch}-${os}")
      endif()

      set(CMAKE_LIBRARY_ARCHITECTURE
          "${lib_arch}"
          CACHE INTERNAL "Guessed library architecture")
    endif()
    set(os_string "${CMAKE_LIBRARY_ARCHITECTURE}")
    set(lib_suffix "${CMAKE_SHARED_MODULE_SUFFIX}")
    set(python_interp "cpython-")
    set(undefined_link_flag "-Wl,--unresolved-symbols=ignore-all")
  elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(rel_rpath_name "")
    # Running Windows 11 in a VM on an ARM-based MacOS shows that cPython's `platform.machine`
    # function correctly returns `ARM64`. On the other hand, Python's library loading mechanism
    # itself is specified by `importlib.machinery.EXTENSION_SUFFIXES`, which returns
    # `cpX.Y-win_amd64.pyd`, so the below should also work on Windows on ARM.
    set(os_string "win_amd64")
    set(lib_suffix ".pyd")
    set(python_interp "cp")
    set(WINDOWS_EXPORT_ALL_SYMBOLS ON)
    # On Windows we need to explicitly link to Python
    target_link_libraries(${ARG_TARGET} PRIVATE msvcrt ${nrnlib})
  else()
    message(
      WARNING
        "Unknown platform ${CMAKE_SYSTEM_NAME}; the built Python library may not function properly")
  endif()

  if(ARG_INSTALL_REL_RPATH)
    # the INSTALL_RPATH property is special in that we cannot set it to the literal `$ORIGIN` in any
    # other way due to the fact that CMake will always escape it (for instance, using
    # target_link_options(target "-Wl,-rpath,\$ORIGIN/some/path") does not work as intended), so
    # this is the only way to actually set it
    set_target_properties(${ARG_TARGET} PROPERTIES INSTALL_RPATH
                                                   "${rel_rpath_name}${ARG_INSTALL_REL_RPATH}")
  endif()

  if(ARG_BUILD_REL_RPATH)
    set_target_properties(${ARG_TARGET} PROPERTIES BUILD_RPATH
                                                   "${rel_rpath_name}${ARG_BUILD_REL_RPATH}")
  endif()

  # MinGW does not play nicely with cython, see: https://github.com/cython/cython/issues/3405
  if(MINGW)
    target_compile_definitions(${ARG_TARGET} PUBLIC MS_WIN64=1)
  endif()

  # the linker should ignore undefined symbols since we cannot link to libpython
  target_link_options(${ARG_TARGET} PRIVATE "${undefined_link_flag}")

  # set library name and output dir
  string(REPLACE "." "" pyver_nodot "${ARG_PYTHON_VERSION}")

  if(ARG_NO_EXTENSION)
    set(output_name "${name}")
  else()
    if(ARG_HAS_FREE_THREADING)
      # the free-threaded build needs an additional `t` after the Python version
      set(pyver_nodot "${pyver_nodot}t")
    endif()
    set(output_name "${name}.${python_interp}${pyver_nodot}-${os_string}")
  endif()

  set_target_properties(
    ${ARG_TARGET}
    PROPERTIES OUTPUT_NAME "${output_name}"
               LINKER_LANGUAGE ${ARG_LANGUAGE}
               PREFIX ""
               SUFFIX ${lib_suffix}
               LIBRARY_OUTPUT_DIRECTORY ${ARG_OUTPUT_DIR})

endfunction()

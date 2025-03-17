# cmake-format: off
# Helper for compiling Python modules currently used for the following modules:
# - neuron.hoc
# - neuron.rxd.geometry3d.surfaces
# - neuron.rxd.geometry3d.ctng
# - neuron.rxd.geometry3d.graphicsPrimitives
# - neuron music module
# cmake-format: on

# convert a pyx file into a c or cpp file
function(cythonize input_file)
  cmake_parse_arguments(ARG "" "LANGUAGE;PYTHON_EXECUTABLE;OUTPUT" "" ${ARGN})
  string(TOLOWER "${ARG_LANGUAGE}" ARG_LANGUAGE)
  if(ARG_LANGUAGE STREQUAL "c++")
    add_custom_command(
      OUTPUT ${ARG_OUTPUT}
      COMMAND ${ARG_PYTHON_EXECUTABLE} -m cython --cplus ${input_file} --output-file ${ARG_OUTPUT}
      DEPENDS ${input_file}
      VERBATIM)
  elseif(ARG_LANGUAGE STREQUAL "c")
    add_custom_command(
      OUTPUT ${ARG_OUTPUT}
      COMMAND ${ARG_PYTHON_EXECUTABLE} -m cython ${input_file} --output-file ${ARG_OUTPUT}
      DEPENDS ${input_file}
      VERBATIM)
  else()
    message(FATAL_ERROR "Unsupported language: ${ARG_LANGUAGE}; only c and c++ are allowed")
  endif()
endfunction()

# create a target that links to nrnpython
function(add_nrn_python_library name)
  cmake_parse_arguments(ARG "" "TARGET;PYTHON_VERSION;LANGUAGE;OUTPUT_DIR"
                        "SOURCES;INCLUDES;LIBRARIES;REL_RPATH" ${ARGN})

  add_library(${ARG_TARGET} SHARED ${ARG_SOURCES})
  if(ARG_INCLUDES)
    target_include_directories(${ARG_TARGET} PRIVATE ${ARG_INCLUDES})
  endif()
  if(ARG_LIBRARIES)
    target_link_libraries(${ARG_TARGET} PRIVATE ${ARG_LIBRARIES})
  endif()

  # set platform-specific config
  if(APPLE)
    set(rel_rpath "@loader_path")
    set(os_string "darwin")
    set(lib_suffix ".so")
    set(python_interp "cpython-")
  elseif(UNIX AND NOT APPLE)
    set(rel_rpath "\\\$ORIGIN")
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
    set(lib_suffix ".so")
    set(python_interp "cpython-")
  elseif(WIN32)
    set(rel_rpath "")
    set(os_string "win_amd64")
    set(lib_suffix ".pyd")
    set(python_interp "cp")
  else()
    message(WARNING "Unknown platform; RPATH may not be set correctly")
  endif()
  if(ARG_REL_RPATH)
    target_link_options(${ARG_TARGET} PRIVATE "-Wl,-rpath,${rel_rpath}${ARG_REL_RPATH}")
  endif()

  # MinGW does not play nicely with cython
  if(MINGW)
    target_compile_definitions(${ARG_TARGET} PUBLIC MS_WIN64=1)
  endif()

  # set library name and output dir
  string(REPLACE "." "" pyver_nodot "${ARG_PYTHON_VERSION}")
  set_target_properties(
    ${ARG_TARGET}
    PROPERTIES OUTPUT_NAME "${name}.${python_interp}${pyver_nodot}-${os_string}"
               LINKER_LANGUAGE ${ARG_LANGUAGE}
               PREFIX ""
               SUFFIX ${lib_suffix}
               LIBRARY_OUTPUT_DIRECTORY ${ARG_OUTPUT_DIR})

endfunction()

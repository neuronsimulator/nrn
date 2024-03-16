# Disable some of the more pedantic checks. float-divide-by-zero is undefined in the C/C++ standard
# but defined by (recent?) Clang. This is disabled for convenience because some user MOD files
# trigger it, while the NEURON codebase does not seem to do so itself.
# implicit-signed-integer-truncation does not flagged undefined behaviour, but rather behaviour that
# may be unintentional; unsigned-integer-overflow is similarly not actually undefined -- these
# checks are not enabled because they cause too much noise at the moment.
set(${CODING_CONV_PREFIX}_SANITIZERS_UNDEFINED_EXCLUSIONS
    float-divide-by-zero implicit-signed-integer-truncation unsigned-integer-overflow
    CACHE STRING "" FORCE)
include("${CODING_CONV_CMAKE}/sanitizers.cmake")
include(${CODING_CONV_CMAKE}/build-time-copy.cmake)

# Propagate the sanitizer flags to the NEURON sources
list(APPEND NRN_COMPILE_FLAGS ${NRN_SANITIZER_COMPILER_FLAGS})
list(APPEND NRN_LINK_FLAGS ${NRN_SANITIZER_COMPILER_FLAGS})
# And to CoreNEURON as we don't have CORENRN_SANITIZERS any more
list(APPEND CORENRN_EXTRA_CXX_FLAGS ${NRN_SANITIZER_COMPILER_FLAGS})
list(APPEND CORENRN_EXTRA_MECH_CXX_FLAGS ${NRN_SANITIZER_COMPILER_FLAGS})
list(APPEND CORENRN_EXTRA_LINK_FLAGS ${NRN_SANITIZER_COMPILER_FLAGS})
if(NRN_SANITIZER_LIBRARY_DIR)
  # At least Clang 14 does not add rpath entries for the sanitizer runtime libraries. Adding this
  # argument saves having to carefully set LD_LIBRARY_PATH and friends.
  list(APPEND NRN_LINK_FLAGS "-Wl,-rpath,${NRN_SANITIZER_LIBRARY_DIR}")
  list(APPEND CORENRN_EXTRA_LINK_FLAGS "-Wl,-rpath,${NRN_SANITIZER_LIBRARY_DIR}")
endif()
if(NRN_SANITIZERS)
  # nocmodl is quite noisy when run under LeakSanitizer, so only set the environment variables that
  # enable sanitizers if LeakSanitizer is not among them
  string(REPLACE "," ";" nrn_sanitizers "${NRN_SANITIZERS}") # NRN_SANITIZERS is comma-separated
  if("leak" IN_LIST nrn_sanitizers)
    set(NRN_NOCMODL_SANITIZER_ENVIRONMENT ${NRN_SANITIZER_DISABLE_ENVIRONMENT})
  else()
    set(NRN_NOCMODL_SANITIZER_ENVIRONMENT ${NRN_SANITIZER_ENABLE_ENVIRONMENT})
  endif()
  if("address" IN_LIST nrn_sanitizers)
    list(APPEND NRN_COMPILE_DEFS NRN_ASAN_ENABLED)
  endif()
  if("thread" IN_LIST nrn_sanitizers)
    list(APPEND NRN_COMPILE_DEFS NRN_TSAN_ENABLED)
  endif()
  # generate and install a launcher script called nrn-enable-sanitizer [--preload] that sets
  # *SAN_OPTIONS variables and, optionally, LD_PRELOAD -- this is useful both in CI configuration
  # and when using the sanitizers "downstream" of NEURON
  string(JOIN " " NRN_SANITIZER_ENABLE_ENVIRONMENT_STRING ${NRN_SANITIZER_ENABLE_ENVIRONMENT})
  # sanitizer suppression files are handled similarly to other data files: we copy them to the same
  # path under the build and installation directories, and then assume the installation path unless
  # the NRNHOME environment variable is set, in which case it is assumed to point to the build
  # directory
  foreach(sanitizer ${nrn_sanitizers})
    if(EXISTS "${PROJECT_SOURCE_DIR}/.sanitizers/${sanitizer}.supp")
      configure_file("${PROJECT_SOURCE_DIR}/.sanitizers/${sanitizer}.supp"
                     "${PROJECT_BINARY_DIR}/share/nrn/sanitizers/${sanitizer}.supp" COPYONLY)
      install(FILES "${PROJECT_BINARY_DIR}/share/nrn/sanitizers/${sanitizer}.supp"
              DESTINATION "${CMAKE_INSTALL_PREFIX}/share/nrn/sanitizers")
    endif()
  endforeach()
  # sanitizers.cmake uses absolute paths to suppression files in build directories. substitute that
  # with NEURON-specific NRNHOME-enabled logic
  string(REPLACE "${PROJECT_SOURCE_DIR}/.sanitizers/" "\${prefix}/share/nrn/sanitizers/"
                 NRN_SANITIZER_ENABLE_ENVIRONMENT_STRING
                 "${NRN_SANITIZER_ENABLE_ENVIRONMENT_STRING}")
  if(NRN_SANITIZER_LIBRARY_PATH)
    set(NRN_SANITIZER_LD_PRELOAD "${NRN_SANITIZER_PRELOAD_VAR}=${NRN_SANITIZER_LIBRARY_PATH}")
  endif()
endif()

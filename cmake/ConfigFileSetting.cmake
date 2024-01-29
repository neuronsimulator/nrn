# =============================================================================
# Definitions used in nrnconf.h and nmodlconf.h
# =============================================================================
set(PACKAGE_NAME "${PACKAGE}")
set(PACKAGE_TARNAME "${PACKAGE}")
set(PACKAGE_BUGREPORT "\"\"")
set(PACKAGE_URL "\"\"")
set(UNQUOTED_PACKAGE_VERSION "${PROJECT_VERSION}")

# ~~~
# some of the variables need to be double quoted strings as they are
# used in the above mentioned template files name for libraries
# ~~~
nrn_set_string(PACKAGE "nrn")
nrn_set_string(NRNHOST "${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_NAME}")
nrn_set_string(PACKAGE_STRING "nrn ${PROJECT_VERSION}")
nrn_set_string(PACKAGE_VERSION "${PROJECT_VERSION}")
nrn_set_string(VERSION "${PROJECT_VERSION}")
nrn_set_string(NRN_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
nrn_set_string(NEURON_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/nrn")
nrn_set_string(DLL_DEFAULT_FNAME "${CMAKE_SYSTEM_PROCESSOR}/.libs/libnrnmech.so")

# indicate nmodl config is used
add_definitions(-DHAVE_CONFIG_H)

set(YYTEXT_POINTER 1)
set(TIME_WITH_SYS_TIME 1)
# below two are universal nowadays
set(prefix ${CMAKE_INSTALL_PREFIX})
set(host_cpu ${CMAKE_SYSTEM_PROCESSOR})
set(exec_prefix ${prefix})
set(bindir \${exec_prefix}/bin)
set(modsubdir ${host_cpu})
set(bindir \${exec_prefix}/bin)
set(libdir \${exec_prefix}/lib)

# =============================================================================
# Comment or empty character to enable/disable cmake specific settings
# =============================================================================
# adding `#` makes a comment in the python whereas empty enable code
if(NRN_ENABLE_CORENEURON)
  set(CORENEURON_ENABLED_TRUE "")
  set(CORENEURON_ENABLED_FALSE "#")
else()
  set(CORENEURON_ENABLED_TRUE "#")
  set(CORENEURON_ENABLED_FALSE "")
endif()

# ~~~
# A variable that doesn't start out as #undef but as #define needs an
# explicit @...@ replacement in the .h.in files.
# The value is set to description followed by a string of space
# separated 'option=value' where value differs from the default value.
# ~~~
set(neuron_config_args "cmake option default differences:")
set(neuron_config_args_all)
foreach(_name ${NRN_OPTION_NAME_LIST})
  if(NOT ("${${_name}}" STREQUAL "${${_name}_DEFAULT}"))
    string(APPEND neuron_config_args " '${_name}=${${_name}}'")
  endif()
  string(REPLACE ";" "\\;" escaped_value "${${_name}}")
  string(REPLACE "\"" "\\\"" escaped_value "${escaped_value}")
  list(APPEND neuron_config_args_all "{\"${_name}\", \"${escaped_value}\"}")
endforeach()
string(JOIN ",\n  " neuron_config_args_all ${neuron_config_args_all})

# =============================================================================
# Platform specific options (get expanded to comments)
# =============================================================================
# for nrn.defaults
set(nrndef_unix "//")
set(nrndef_mac "//")
set(nrndef_mswin "//")
set(NRN_OSX_BUILD_TRUE "#")

if(NRN_LINUX_BUILD)
  set(nrndef_unix "")
elseif(NRN_MACOS_BUILD)
  set(nrndef_mac "")
  set(DARWIN 1)
  set(NRN_OSX_BUILD_TRUE "")
elseif(NRN_WINDOWS_BUILD)
  set(nrndef_mswin "")
endif()

# =============================================================================
# Options based on user provided build options
# =============================================================================
if(NRN_ENABLE_DISCRETE_EVENT_OBSERVER)
  set(DISCRETE_EVENT_OBSERVER 1)
else()
  set(DISCRETE_EVENT_OBSERVER 0)
endif()

if(NRN_ENABLE_MECH_DLL_STYLE)
  set(NRNMECH_DLL_STYLE 1)
else()
  unset(NRNMECH_DLL_STYLE)
endif()

if(NRN_ENABLE_INTERVIEWS AND NOT MINGW)
  set(NRNOC_X11 1)
else()
  set(NRNOC_X11 0)
endif()

if(NRN_ENABLE_PYTHON_DYNAMIC)
  list(APPEND NRN_COMPILE_DEFS NRNPYTHON_DYNAMICLOAD)
endif()

# =============================================================================
# Dependencies option
# =============================================================================
set(SUNDIALS_DOUBLE_PRECISION 1)
set(SUNDIALS_USE_GENERIC_MATH 1)

# =============================================================================
# Similar to check_include_files but also construct NRN_HEADERS_INCLUDE_LIST
# =============================================================================
nrn_check_include_files(dlfcn.h HAVE_DLFCN_H)
nrn_check_include_files(execinfo.h HAVE_EXECINFO_H)
nrn_check_include_files(fenv.h HAVE_FENV_H)
nrn_check_include_files(malloc.h HAVE_MALLOC_H)
nrn_check_include_files(strings.h HAVE_STRINGS_H)
nrn_check_include_files(sys/types.h HAVE_SYS_TYPES_H)
nrn_check_include_files(unistd.h HAVE_UNISTD_H)

# =============================================================================
# Check for standard headers
# =============================================================================
check_include_files("dlfcn.h;stdint.h;stddef.h;inttypes.h;stdlib.h;strings.h;string.h;float.h"
                    STDC_HEADERS)

# =============================================================================
# Check symbol using check_cxx_symbol_exists but use ${NRN_HEADERS_INCLUDE_LIST}
# =============================================================================
# note that this must be called after all *check_include_files because we use
# NRN_HEADERS_INCLUDE_LIST is second argument (headers) is empty.
nrn_check_symbol_exists("bcopy" "strings.h" HAVE_BCOPY)
nrn_check_symbol_exists("bzero" "strings.h" HAVE_BZERO)
nrn_check_symbol_exists("gettimeofday" "sys/time.h" HAVE_GETTIMEOFDAY)
nrn_check_symbol_exists("index" "strings.h" HAVE_INDEX)
nrn_check_symbol_exists("isatty" "unistd.h" HAVE_ISATTY)
nrn_check_symbol_exists("iv" "" HAVE_IV)
nrn_check_symbol_exists("mallinfo" "malloc.h" HAVE_MALLINFO)
nrn_check_symbol_exists("mallinfo2" "malloc.h" HAVE_MALLINFO2)
nrn_check_symbol_exists("mkstemp" "stdlib.h" HAVE_MKSTEMP)
nrn_check_symbol_exists("posix_memalign" "stdlib.h" HAVE_POSIX_MEMALIGN)
nrn_check_symbol_exists("realpath" "stdlib.h" HAVE_REALPATH)
nrn_check_symbol_exists("setenv" "stdlib.h" HAVE_SETENV)
nrn_check_symbol_exists("setitimer" "sys/time.h" HAVE_SETITIMER)
nrn_check_symbol_exists("sigaction" "signal.h" HAVE_SIGACTION)
nrn_check_symbol_exists("sigprocmask" "signal.h" HAVE_SIGPROCMASK)
nrn_check_symbol_exists("SIGBUS" "signal.h" HAVE_SIGBUS)
nrn_check_symbol_exists("stty" "" HAVE_STTY)
nrn_check_cxx_symbol_exists("fesetround" "fenv.h" HAVE_FESETROUND)
nrn_check_cxx_symbol_exists("feenableexcept" "fenv.h" HAVE_FEENABLEEXCEPT)

# =============================================================================
# Check data types
# =============================================================================
nrn_check_type_exists(sys/types.h gid_t int gid_t)
nrn_check_type_exists(sys/types.h off_t "long int" off_t)
nrn_check_type_exists(sys/types.h pid_t int pid_t)
nrn_check_type_exists(sys/types.h size_t "unsigned int" size_t)
nrn_check_type_exists(sys/types.h uid_t int uid_t)

# =============================================================================
# Set return type of signal in RETSIGTYPE
# =============================================================================
nrn_check_signal_return_type(RETSIGTYPE)

# =============================================================================
# Check direcotry manipulation header
# =============================================================================
nrn_check_dir_exists(sys/ndir.h HAVE_SYS_NDIR_H)

# =============================================================================
# Copy cmake specific template files
# =============================================================================
# nrnconf.h.in and nmodlconf.h.in were originally generated from config.h.in generated by
# autoheader. We use repository copy cmake_nrnconf.h.in to create nrnconf.h and nmodlconf.h directly
# in the PROJECT_BINARY_DIR from the nrn_configure_dest_src macro.

# =============================================================================
# Generate file from file.in template
# =============================================================================
set(version_strs ${NRN_PYTHON_VERSIONS})
list(TRANSFORM version_strs APPEND "\"")
list(TRANSFORM version_strs PREPEND "\"")
string(JOIN ", " NRN_DYNAMIC_PYTHON_LIST_OF_VERSION_STRINGS ${version_strs})
nrn_configure_dest_src(nrnconf.h . cmake_nrnconf.h .)
nrn_configure_dest_src(nmodlconf.h . cmake_nrnconf.h .)
nrn_configure_file(nrnmpiuse.h src/oc)
nrn_configure_file(nrnconfigargs.h src/nrnoc)
nrn_configure_file(nrnneosm.h src/nrncvode)
nrn_configure_file(sundials_config.h src/sundials)
nrn_configure_dest_src(nrn.defaults share/nrn/lib nrn.defaults share/lib)
file(COPY ${PROJECT_SOURCE_DIR}/share/lib/nrnunits.lib
     DESTINATION ${PROJECT_BINARY_DIR}/share/nrn/lib)

if(NRN_MACOS_BUILD)
  set(abs_top_builddir ${PROJECT_BINARY_DIR})
  nrn_configure_file(macdist.pkgproj src/mac)
  nrn_configure_file(postinstall.sh src/mac)
endif()
if(MINGW)
  dospath("${CMAKE_INSTALL_PREFIX}" WIN_MARSHAL_NRN_DIR)
  nrn_configure_file(nrnsetupmingw.nsi src/mswin)
  nrn_configure_file(pre_setup_exe.sh src/mswin)
  # Just name and not path since setup.exe user chooses location of install.
  set(CXX x86_64-w64-mingw32-g++.exe)
  set(BUILD_MINGW_TRUE "")
  set(BUILD_MINGW_FALSE "#")
  set(nrnskip_rebase "#")
  nrn_configure_file(mknrndll.mak src/mswin/lib)
endif()

# =============================================================================
# If Interviews is not provided, configure local files
# =============================================================================
if(NOT NRN_ENABLE_INTERVIEWS)
  nrn_configure_dest_src(config.h . cmake_nrnconf.h .)
else()
  file(REMOVE "${PROJECT_BINARY_DIR}/config.h")
endif()

# Prepare some variables for setup.py extension building (hoc_module, rx3d and music)
string(JOIN " " NRN_COMPILE_FLAGS_STRING ${NRN_COMPILE_FLAGS})
string(JOIN " " NRN_LINK_FLAGS_STRING ${NRN_LINK_FLAGS})

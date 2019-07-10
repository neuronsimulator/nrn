cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)
project(nrn C CXX)

# =============================================================================
# CMake common project settings
# =============================================================================
set(PROJECT_VERSION_MAJOR 7)
set(PROJECT_VERSION_MINOR 7)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#some nrnconf.h and nmodlconf.h definitions
set(NRNHOST "\"${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_NAME}\"")
set(NRNHOSTCPU "\"${CMAKE_SYSTEM_PROCESSOR}\"")
set(PACKAGE "\"nrn\"")
set(PACKAGE_BUGREPORT "\"\"")
set(PACKAGE_NAME "${PACKAGE}")
set(PACKAGE_STRING "\"nrn ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}\"")
set(PACKAGE_TARNAME "${PACKAGE}")
set(PACKAGE_URL "\"\"")
set(PACKAGE_VERSION "\"${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}\"")
set(VERSION "\"${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}\"")
set(NRN_LIBDIR "\"${CMAKE_INSTALL_PREFIX}/${CMAKE_SYSTEM_PROCESSOR}/lib\"")
set(NEURON_DATA_DIR "\"${CMAKE_INSTALL_PREFIX}/share/nrn\"")
set(SUNDIALS_DOUBLE_PRECISION 1)
set(SUNDIALS_USE_GENERIC_MATH 1)
set(LT_OBJDIR "\".libs/\"")
#INCLUDE(TestBigEndian)
#TEST_BIG_ENDIAN(WORDS_BIGENDIAN)
set(YYTEXT_POINTER 1)
set(TIME_WITH_SYS_TIME 1)
set(HAVE_NAMESPACES "/**/")
set(HAVE_STTY 0)
set(IVOS_FABS "::fabs") # complicated configure test but I don't think this is ever not the case anymore
set(HAVE_STL "/**/") # nowadays universal for CXX
set(DLL_DEFAULT_FNAME "\"${CMAKE_SYSTEM_PROCESSOR}/.libs/libnrnmech.so\"")

# for nrn.defaults
set(nrndef_unix "//")
set(nrndef_mac "//")
set(nrndef_mswin "//")
set(MAC_DARWIN_TRUE "\#")
set(MAC_DARWIN_FALSE "")
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(nrndef_unix "")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(nrndef_mac "")
  set(MAC_DARWIN_TRUE "")
  set(MAC_DARWIN_FALSE "\#")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(nrndef_mswin "")
endif()

#for nrnmech_makefile
set(CMAKE_INSTALL_BINDIR bin)
set(CMAKE_INSTALL_LIBDIR lib)
set(CMAKE_INSTALL_INCLUDEDIR include)
set(CMAKE_INSTALL_DATADIR share/nrn)
#------modified coreneuron/extra/CMakeLists.txt
include(${CMAKE_CURRENT_SOURCE_DIR}/bin/CMakeListsNrnMech.txt)
#------end of modified coreneuron/extra/CMakeLists.txt

option(NRN_BUILD_SHARED "Build libraries shared or static" OFF)
option(ENABLE_INTERVIEWS "Link with INTERVIEWS GUI library" ON)
option(ENABLE_LegacyFR "Original faraday, R, etc. instead of 2017 nist constants in share/lib/nrnunits.lib" ON)
option(ENABLE_NRNMECH_DLL_STYLE "Dynamically load nrnmech shared library" ON)
option(ENABLE_DISCRETE_EVENT_OBSERVER "Set to OFF if do not want Observer to be a subclass of DiscreteEvent" ON)
option(ENABLE_PYTHON "Python interpreter available" ON)
option(ENABLE_THREADS "Allow use of pthreads" ON)
option(ENABLE_MPI "Allow use of MPI" ON)

if (ENABLE_DISCRETE_EVENT_OBSERVER)
  set(DISCRETE_EVENT_OBSERVER 1)
else()
  set(DISCRETE_EVENT_OBSERVER 0)
endif()

if (ENABLE_LegacyFR)
  set (LegacyFR 1)
  set (LegacyY "")
  set (LegacyN "/")
else()
  set (LegacyFR 0)
  set (LegacyY "/")
  set (LegacyN "")
endif()

if (ENABLE_NRNMECH_DLL_STYLE)
  set(NRNMECH_DLL_STYLE 1)
else()
  set(NRNMECH_DLL_STYLE 0)
endif()
set(USING_CMAKE_FALSE "#")
set(USING_CMAKE_TRUE "")

#these two don't start out as #undef but as #define so need their explicit
#@...@ replacments
#set(NEURON_BIN_DIR "\"${CMAKE_INSTALL_PREFIX}/${CMAKE_SYSTEM_PROCESSOR}/bin\"")
#set(NRN_CONFIG_ARGS "\"unknown\"") # ends up as "" since no @...@
set (prefix ${CMAKE_INSTALL_PREFIX})
set (host_cpu ${CMAKE_SYSTEM_PROCESSOR})

set (exec_prefix ${prefix})
set (bindir \${exec_prefix}/bin)
set (modsubdir ${host_cpu})

# =============================================================================
# Compile static libraries with hidden visibility
# =============================================================================
#set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# =============================================================================
# Find required packages
# =============================================================================
message(STATUS "CHECKING FOR X11")
find_package(BISON)
find_package(FLEX)

if (ENABLE_MPI)
  find_package(MPI REQUIRED)
  set(NRNMPI 1)
  set(PARANEURON 1)
else()
  set(NRNMPI 0)
  set(PARANEURON 0)
endif()

if (ENABLE_INTERVIEWS)
  find_package(X11)
  if(NOT ${X11_FOUND})
    if (APPLE)
      message(FATAL_ERROR "You must install XQuartz from https://www.xquartz.org/ to build iv")
    else()
      find_package(X11 REQUIRED)
    endif()
  endif()

  find_package(iv REQUIRED)
  set (HAVE_IV 1)
  set (NRNOC_X11 1)
else()
  set (HAVE_IV 0)
  set (NRNOC_X11 0)
endif()

# python or python3 is also needed to make hocusr.h from neuron.h
find_package(PythonInterp REQUIRED) #currently active
message(NOTICE "ENABLE_PYTHON ${ENABLE_PYTHON}")
if (ENABLE_PYTHON)
  message(NOTICE " inside ENABLE_PYTHON")
  find_package(PythonLibs REQUIRED) # consistent with above
  set(USE_PYTHON 1)
  message(NOTICE "USE_PYTHON ${USE_PYTHON}")
else()
  set(USE_PYTHON 0)
endif()

if (ENABLE_THREADS)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  set(USE_PTHREAD 1)
else()
  set(USE_PTHREAD 0)
endif()

# =============================================================================
# Include cmake modules
# =============================================================================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(RpathHelper)
include(ConfigACFile)

include_directories(${PROJECT_SOURCE_DIR}/src/include)

# =============================================================================
# Project version from git and project directories
# =============================================================================
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

add_definitions(-DHAVE_CONFIG_H)

have_dir(dirent.h HAVE_DIRENT_H)
have_dir(ndir.h HAVE_NDIR_H)
have_dir(sys/dir.h HAVE_SYS_DIR_H)
have_dir(sys/ndir.h HAVE_SYS_NDIR_H)
if (HAVE_DIRENT_H) # autoconf sets to 0 in this case
  set(HAVE_SYS_DIR_H 0)
endif()


# like check_include_files but makes a INC_FILE_LIST of those that exist
my_check_include_files(alloca.h HAVE_ALLOCA_H)
my_check_include_files(dlfcn.h HAVE_DLFCN_H)
my_check_include_files(fcntl.h HAVE_FCNTL_H)
my_check_include_files(fenv.h HAVE_FENV_H)
my_check_include_files(float.h HAVE_FLOAT_H)
my_check_include_files(inttypes.h HAVE_INTTYPES_H)
my_check_include_files(limits.h HAVE_LIMITS_H) 
my_check_include_files(locale.h HAVE_LOCALE_H)
my_check_include_files(malloc.h HAVE_MALLOC_H)
my_check_include_files(math.h HAVE_MATH_H)
my_check_include_files(memory.h HAVE_MEMORY_H)
my_check_include_files(pthread.h HAVE_PTHREAD_H)
my_check_include_files(sgtty.h HAVE_SGTTY_H)
my_check_include_files(stdarg.h HAVE_STDARG_H)
my_check_include_files(stdint.h HAVE_STDINT_H)
my_check_include_files(stdlib.h HAVE_STDLIB_H)
my_check_include_files(stream.h HAVE_STREAM_H)
my_check_include_files(strings.h HAVE_STRINGS_H)
my_check_include_files(string.h HAVE_STRING_H)
my_check_include_files(stropts.h HAVE_STROPTS_H)
my_check_include_files(sys/conf.h HAVE_SYS_CONF_H)
my_check_include_files(sys/file.h HAVE_SYS_FILE_H)
my_check_include_files(sys/ioctl.h HAVE_SYS_IOCTL_H)
my_check_include_files(sys/stat.h HAVE_SYS_STAT_H)
my_check_include_files(sys/time.h HAVE_SYS_TIME_H)
my_check_include_files(sys/types.h HAVE_SYS_TYPES_H)
my_check_include_files(sys/wait.h HAVE_SYS_WAIT_H)
my_check_include_files(termio.h HAVE_TERMIO_H)
my_check_include_files(unistd.h HAVE_UNISTD_H)
my_check_include_files(varargs.h HAVE_VARARGS_H)
my_check_include_files(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_files("dlfcn.h;stdint.h;stddef.h;inttypes.h;stdlib.h;strings.h;string.h;float.h" STDC_HEADERS)
check_include_files("_G_config.h" HAVE__G_CONFIG_H LANGUAGE CXX)

message(STATUS "INC_FILE_LIST ${INC_FILE_LIST}")

#like check_symbol_exists but internally <files> arg is "${INC_FILE_LIST}"
#unless not empty.
my_check_symbol_exists("alloca" "" HAVE_ALLOCA)
my_check_symbol_exists("bcopy" "" HAVE_BCOPY)
my_check_symbol_exists("bzero" "" HAVE_BZERO)
my_check_symbol_exists("doprnt" "" HAVE_DOPRNT)
#my_check_symbol_exists("DIR" "dirent.h" HAVE_DIRENT_H) #see have_dir above
my_check_cxx_symbol_exists("fesetround" "" HAVE_FESETROUND)
my_check_symbol_exists("ftime" "" HAVE_FTIME)
my_check_symbol_exists("getcwd" "" HAVE_GETCWD)
my_check_symbol_exists("gethostname" "" HAVE_GETHOSTNAME)
my_check_cxx_symbol_exists("getpw" "sys/types.h;pwd.h" HAVE_GETPW)
my_check_symbol_exists("gettimeofday" "" HAVE_GETTIMEOFDAY)
my_check_symbol_exists("index" "" HAVE_INDEX)
my_check_symbol_exists("isatty" "" HAVE_ISATTY)
my_check_symbol_exists("iv" "" HAVE_IV)
my_check_symbol_exists("lockf" "" HAVE_LOCKF)
my_check_symbol_exists("mallinfo" "" HAVE_MALLINFO)
my_check_symbol_exists("mkdir" "" HAVE_MKDIR)
my_check_symbol_exists("mkstemp" "" HAVE_MKSTEMP)
my_check_symbol_exists("namespaces" "" HAVE_NAMESPACES)
my_check_symbol_exists("posix_memalign" "" HAVE_POSIX_MEMALIGN)
my_check_symbol_exists("pthread" "" HAVE_PTHREAD)
my_check_symbol_exists("putenv" "" HAVE_PUTENV)
my_check_symbol_exists("realpath" "" HAVE_REALPATH)
my_check_symbol_exists("select" "" HAVE_SELECT)
my_check_symbol_exists("setenv" "" HAVE_SETENV)
my_check_symbol_exists("setitimer" "" HAVE_SETITIMER)
my_check_symbol_exists("sigaction" "" HAVE_SIGACTION)
my_check_symbol_exists("SIGBUS" "signal.h" HAVE_SIGBUS)
my_check_symbol_exists("SIGSEGV" "signal.h" HAVE_SIGSEGV)
#my_check_cxx_symbol_exists("std::stringstream" "sstream" HAVE_SSTREAM)
set(HAVE_SSTREAM /**/) # should always exist these days
my_check_symbol_exists("strdup" "" HAVE_STRDUP)
my_check_symbol_exists("strstr" "" HAVE_STRSTR)
my_check_symbol_exists("stty" "" HAVE_STTY)
my_check_symbol_exists("vprintf" "" HAVE_VPRINTF)

setretsigtype() # very likely void

have_type(sys/types.h gid_t int gid_t)
have_type(sys/types.h off_t "long int" off_t)
have_type(sys/types.h pid_t int pid_t)
have_type(sys/types.h size_t "unsigned int" size_t)
have_type(sys/types.h uid_t int uid_t)


if(${CMAKE_SYSTEM_NAME} MATCHES "CYGWIN")
  set(CYGWIN 1)
endif()

# generate foo from foo.in files
configure_ac_file(nrnconf.h .)
#Note:until the autoconf configure becomes operational in cmake,
#just copy the autoconf .h files derived from .h.in into the
#proper build locations.


install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/lib
  DESTINATION share/nrn)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/demo
  DESTINATION share/nrn)
install(FILES ${PROJECT_BINARY_DIR}/share/lib/nrnunits.lib
  ${PROJECT_BINARY_DIR}/share/lib/nrn.defaults
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/nrn/lib
)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/share/lib/cleanup
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/nrn/lib
)

set(NRNMECH_DLL_STYLE_FALSE "\#")
set(NRNMECH_DLL_STYLE_TRUE "")
configure_ac_file(nrngui bin)
configure_ac_file(neurondemo bin)
configure_ac_file(nrnivmodl bin)
#nrnmech_makefile.in is intimately bound up with autoconf (libtool)
#configure_ac_file(nrnmech_makefile bin)
#so instead use a modified copy of the CoreNEURON version of
#nrnivmodl_core_makefile.in
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/nrnivmodl_makefile.cmk
  ${PROJECT_BINARY_DIR}/bin/nrnmech_makefile
  @ONLY
)

install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/nrngui
  ${PROJECT_BINARY_DIR}/bin/neurondemo
  ${PROJECT_BINARY_DIR}/bin/nrnivmodl
  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
install(FILES ${PROJECT_BINARY_DIR}/bin/nrnmech_makefile
  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

# paths to *.h files for installing into <prefix>/include
my_find_files(inst_inc
  cabvars.h    hoc_membf.h   neuron.h      nrnjava.h     options.h
  cspmatrix.h  hocparse.h    nmodlmutex.h  nrnoc_ml.h    scoplib_ansi.h
  cspredef.h   md1redef.h    nrn_ansi.h    scoplib.h
  hocassrt.h   md2redef.h    nrnapi.h      nrnran123.h   section.h
  hocdec.h     membdef.h     nrnassrt.h    nrnredef.h    spconfig.h
  hocgetsym.h  membfunc.h    nrncvode.h    oc_ansi.h     spmatrix.h
  hoc.h        multicore.h   nrnisaac.h    ocfunc.h
  hoclist.h    multisplit.h  nrniv_mf.h    ocmisc.h
)
install(FILES ${inst_inc} ${PROJECT_BINARY_DIR}/src/oc/nrnpthread.h
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

add_subdirectory(src/nrniv)

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


option(NRN_BUILD_SHARED "Build libraries shared or static" OFF)


# =============================================================================
# Compile static libraries with hidden visibility
# =============================================================================
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# =============================================================================
# Find required packages
# =============================================================================
message(STATUS "CHECKING FOR X11")
find_package(X11)

if(NOT ${X11_FOUND})
  if (APPLE)
    message(FATAL_ERROR "You must install XQuartz from https://www.xquartz.org/ to build iv")
  endif()
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
my_check_include_files(_g_config.h HAVE__G_CONFIG_H)
my_check_include_files(sys/timeb.h HAVE_SYS_TIMEB_H)

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
set(HAVE_SSTREAM 1) # should alwasy exist these days
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


install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/include/
  DESTINATION include)

add_subdirectory(src/nrniv)

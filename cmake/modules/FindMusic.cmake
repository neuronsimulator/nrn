#
# Find the MUSIC installation.
#

if (NOT NRN_ENABLE_MPI)
  message(FATAL "MUSIC requires -DNRN_ENABLE_MPI=ON")
endif()

if (NOT NRN_ENABLE_PYTHON)
  message(FATAL "MUSIC requires -DNRN_ENABLE_PYTHON=ON")
endif()

message(STATUS "MUSIC requires Cython")
find_package(Cython REQUIRED)

set(MUSIC_FOUND TRUE)
set(MUSIC_PREFIX "/home/hines/soft/MUSIC/install")
set(MUSIC_INCDIR "${MUSIC_PREFIX}/include")
set(MUSIC_LIBDIR "${MUSIC_PREFIX}/lib")


#
# Find the MUSIC installation.
#

find_path(MUSIC_INCDIR music.hh PATH_SUFFIXES include)
find_path(MUSIC_LIBDIR libmusic.a PATH_SUFFIXES lib)
find_library(MUSIC_LIBRARY libmusic.a PATHS ${MUSIC_LIBDIR})

find_package_handle_standard_args(MUSIC "Failed to find MUSIC package"
  MUSIC_INCDIR
  MUSIC_LIBDIR
  MUSIC_LIBRARY)

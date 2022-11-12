#
# Find the MUSIC installation.
#

find_path(MUSIC_INCDIR music.hh PATH_SUFFIXES include)
find_path(MUSIC_LIBDIR libmusic.so PATH_SUFFIXES lib lib/x86_64-linux-gnu)
find_library(MUSIC_LIBRARY libmusic.so PATHS ${MUSIC_LIBDIR})

find_package_handle_standard_args(MUSIC "Failed to find MUSIC package" MUSIC_INCDIR MUSIC_LIBDIR
                                  MUSIC_LIBRARY)

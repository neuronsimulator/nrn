set(SOURCES
cfile/expsyn.c
cfile/hh.c
cfile/passive.c
cfile/stim.c
cfile/netstim.c
cfile/nrnran123.c
)

set(LIBRARY coremech)

include_directories(${CMAKE_SOURCE_DIR}/src)
add_library(${LIBRARY} ${SOURCES})

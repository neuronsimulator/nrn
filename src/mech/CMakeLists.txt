set(SOURCES
cfile/nrnran123.c
cfile/expsyn.c
cfile/hh.c
cfile/passive.c
cfile/stim.c
cfile/netstim.c
)

set(LIBRARY coremech)

include_directories(${CMAKE_SOURCE_DIR}/src)
add_library(${LIBRARY} ${SOURCES})

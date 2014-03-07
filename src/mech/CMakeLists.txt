set(COREMECH_PUBLIC_HEADERS ${COMMON_INCLUDES}
)

set(COREMECH_HEADERS
cfile/cabvars.h
cfile/nrnran123.h
cfile/scoplib.h
)

set(COREMECH_SOURCES
${COMMON_SOURCES}
cfile/expsyn.c
cfile/hh.c
cfile/passive.c
cfile/stim.c
cfile/netstim.c
cfile/nrnran123.c
)

set_source_files_properties(cfile/expsyn.c cfile/hh.c cfile/passive.c cfile/stim.c cfile/netstim.c PROPERTIES COMPILE_FLAGS "-Wno-error")

common_library(coremech)

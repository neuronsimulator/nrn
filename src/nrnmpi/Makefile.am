lib_LTLIBRARIES = libnrnmpi.la

nsrc=$(top_srcdir)/src
INCLUDES = -I$(nsrc)/oc @NRN_REALTIME_INCLUDE@
AM_CFLAGS = @MINGW_CFLAG@

libnrnmpi_la_SOURCES = nrnmpi.c bbsmpipack.c mpispike.c nrnrt.c

EXTRA_DIST = bbsmpipack.h nrnmpi_impl.h mpispike.h notes_nrnrt

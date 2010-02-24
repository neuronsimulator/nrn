lib_LTLIBRARIES = libnrnmpi.la

nsrc=$(top_srcdir)/src
INCLUDES = -I$(nsrc)/oc @NRN_REALTIME_INCLUDE@ -I@MUSIC_INCDIR@
AM_CFLAGS = @MINGW_CFLAG@

libnrnmpi_la_SOURCES = nrnmpi.c bbsmpipack.c mpispike.c nrnrt.c

libnrnmpi_la_LIBADD = @MUSIC_LIBLA@
EXTRA_DIST = bbsmpipack.h nrnmpi_impl.h mpispike.h notes_nrnrt

lib_LTLIBRARIES = libnrnmpi.la

nsrc=$(top_srcdir)/src
INCLUDES = -I$(nsrc)/oc @NRN_REALTIME_INCLUDE@ -I@MUSIC_INCDIR@
AM_CFLAGS = @MINGW_CFLAG@

libnrnmpi_la_SOURCES = nrnmpi.c bbsmpipack.c mpispike.c nrnrt.c

CC = @MPICC@
CXX = @MPICXX@

if BUILD_NRNMPI_DYNAMIC
nrnmpi_dynam.h nrnmpi_dynam_cinc: mkdynam.sh nrnmpidec.h
	(cd $(nsrc)/nrnmpi ; sh mkdynam.sh)

all: nrnmpi_dynam.h
endif

libnrnmpi_la_LIBADD = @MUSIC_LIBLA@
EXTRA_DIST = nrnmpidec.h nrnmpi_impl.h mpispike.h notes_nrnrt \
	mkdynam.sh nrnmpi_dynam.c nrnmpi_dynam_stubs.c

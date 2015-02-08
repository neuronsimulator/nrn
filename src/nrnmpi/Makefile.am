lib_LTLIBRARIES = libnrnmpi.la

nsrc=$(top_srcdir)/src
includes = -I$(nsrc)/oc @NRN_REALTIME_INCLUDE@ -I@MUSIC_INCDIR@
AM_CFLAGS = @MINGW_CFLAG@ $(includes)

libnrnmpi_la_SOURCES = nrnmpi.c bbsmpipack.c mpispike.c nrnrt.c

CC = @MPICCnrnmpi@

if BUILD_NRNMPI_DYNAMIC

#NEED_RTLD_GLOBAL = -L$(libdir) -loc -lnrniv
# liboc.so and libnrnoc.so have not been installed yet.
# need to rerun the make after that
NEED_RTLD_GLOBAL=

INC = nrnmpi_dynam.h nrnmpi_dynam_cinc nrnmpi_dynam_wrappers.inc
$(INC): mkdynam.sh nrnmpidec.h
	(cd $(nsrc)/nrnmpi ; sh mkdynam.sh)

$(libnrnmpi_la_OBJECTS): $(INC)

all: $(nsrc)/nrnmpi/nrnmpi_dynam.h
else
NEED_RTLD_GLOBAL =
endif

libnrnmpi_la_LIBADD = @MUSIC_LIBLA@ $(NEED_RTLD_GLOBAL)
EXTRA_DIST = nrnmpidec.h nrnmpi_impl.h mpispike.h notes_nrnrt \
	mkdynam.sh nrnmpi_dynam.c nrnmpi_dynam_stubs.c nrnmpi_def_cinc

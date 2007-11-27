SUBDIRS =
DIST_SUBDIRS = batch extra rdln windll winio wnrnbbs bin lib

EXTRA_DIST = bbsconf.h njconf.h nrnconf.h nrniv.ico nrniv.rc \
	nrnneosm.h mwprefix.h nrncygso.sh

if BUILD_CYGWIN
all-am: nrniv.exe

FORCE:

nrniv.exe: FORCE
	IVLIBDIR="$(IV_LIBDIR)" CFLAGS="$(CFLAGS)" sh nrncygso.sh
endif

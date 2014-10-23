SUBDIRS =
DIST_SUBDIRS = batch extra rdln windll wnrnbbs bin lib

EXTRA_DIST = bbsconf.h njconf.h nrnconf.h nrniv.ico nrniv10.ico nrniv.rc \
	nrnneosm.h mwprefix.h nrncygso.sh nrnsetup.nsi

AM_CFLAGS = @MINGW_CFLAG@

if BUILD_CYGWIN
all-am: nrniv.exe

FORCE:

nrniv.exe: FORCE
	IVLIBDIR="$(IV_LIBDIR)" CFLAGS="$(AM_CFLAGS)" LDFLAGS="$(LDFLAGS)" sh -e nrncygso.sh
endif

if BUILD_MINGW
all-am: nrniv.exe

FORCE:

nrniv.exe: FORCE
	IVLIBDIR="$(IV_LIBDIR)" CFLAGS="$(AM_CFLAGS)" LDFLAGS="$(LDFLAGS)" \
	  LIBS="$(LIBS)" sh -e nrncygso.sh
endif


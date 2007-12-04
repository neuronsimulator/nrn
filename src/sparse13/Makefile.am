## The library that we make:
lib_LTLIBRARIES = libsparse13.la

AM_CFLAGS = @MINGW_CFLAG@

libsparse13_la_SOURCES = spalloc.c spbuild.c spfactor.c spoutput.c \
	spsolve.c sputils.c \
	cspalloc.c cspbuild.c cspfactor.c cspoutput.c \
	cspsolve.c csputils.c

pkginclude_HEADERS = spmatrix.h spconfig.h cspmatrix.h cspredef.h

noinst_HEADERS = spdefs.h




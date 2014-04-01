## The list of programs that we compile.
bin_PROGRAMS = memacs

memacs_SOURCES = main.c
memacs_LDADD = libmemacs.la


## Libraries that we make from this directory:
lib_LTLIBRARIES = libmemacs.la
libmemacs_la_LIBADD = @TERMCAP_LIB@

libmemacs_la_SOURCES = ansi.c basic.c bind.c buffer.c display.c file.c \
	fileio.c line.c lock.c main1.c random.c region.c \
	search.c spawn.c tcap.c termio.c window.c word.c

## The list of header files that also have to go into the distribution.
noinst_HEADERS = ebind.h edef.h efunc.h epath.h estruct.h redef.h intfunc.h

## Extra stuff that should be put in a distribution but doesn't get
## compiled.
EXTRA_DIST = ibm.c

## Basic HOC library:
lib_LTLIBRARIES = liboc.la libocxt.la

AM_CPPFLAGS = -DOOP=1 -DINTERVIEWS=1 -DHOC=1
AM_CFLAGS = @MINGW_CFLAG@

nsrc=$(top_srcdir)/src
INCLUDES = -I$(nsrc)/memacs -I$(nsrc)/sparse $(X_CFLAGS)

RanFiles = isaac64.c mcran4.c nrnisaac.c

liboc_la_SOURCES = parse.y bksub.c getelm.c lineq.c subrows.c prmat.c \
	code2.c debug.c ocerf.c fileio.c ftime.c \
	getsym.c hoc.c hocedit.c math.c nonlin.c \
	list.c regexp.c audit.c symbol.c \
	version.c xred.c parallel.c functabl.c plot.c plt.c axis.c \
	settext.c x.c fmenu.c termio.c isoc99.c \
	scoprand.c $(RanFiles) cygwinprt.c

## MAC OS X does not allow multiple definitions in separate dyld files
## therefore we create a separate library for the oc objects that
## should appear in oc and ivoc but not nrnoc and nrniv
## modlreg.c is another special case since it should be missing only
## in a special so we will put it explicitly into nrnoc and nrniv
libocxt_la_SOURCES = code.c hoc_init.c hoc_oop.c \
	spinit.c spinit1.c spinit2.c

AM_YFLAGS = -d

EXTRA_DIST = hocinc hocusr.c hoc2dec.sh macprt.c mswinprt.c system.c

liboc_la_LIBADD = $(X_LIBS)

bin_PROGRAMS = oc

## modlreg.c is here because the newer libtool no longer
## always makes a .o link to the .lo file
oc_SOURCES = ocmain.c ocnoiv.c nocable.c modlreg.c ockludge.c

oc_LDADD = libocxt.la liboc.la \
	../nrnmpi/libnrnmpi.la \
	@MEMACSLIBLA@ @NRN_READLINE_LIBS@

hocdec.h: hoc.h hoc2dec.sh
	./hoc2dec.sh
	
## Header files that need to be installed:
pkginclude_HEADERS = hoc.h hoclist.h hocdec.h hocassrt.h nrnisaac.h \
	nrnpthread.h

## Header files that need to be copied into the distribution but do not need
## to be installed:
noinst_HEADERS = code.h equation.h hocmodl.h hocstr.h \
	profile.h redef.h parse.h mcran4.h nrnmpi.h nrnrt.h isaac64.h

## files that depend on the existence of parse.h
## this allows parallel make -j to work at the beginning when parse.h
## does not exist and dependencies have not yet been determined
## I am not sure whether both .lo and .o lists are necessary.
## Also, oc/parse.h is included in files in the nrnoc, ivoc, and
## nrniv directories so we hope that parse.h is created before
## make -v gets into those.

code2.lo code.lo fileio.lo getsym.lo hoc.lo hoc_init.lo hoc_oop.lo \
list.lo nonlin.lo parallel.lo symbol.lo: parse.h

code2.o code.o fileio.o getsym.o hoc.o hoc_init.o hoc_oop.o \
list.o nonlin.o parallel.o symbol.o: parse.h


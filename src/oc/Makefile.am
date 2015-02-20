## Basic HOC library:
lib_LTLIBRARIES = liboc.la libocxt.la

nsrc=$(top_srcdir)/src
includes = -I$(nsrc) -I$(nsrc)/memacs -I$(nsrc)/sparse -I$(nsrc)/nrnmpi $(X_CFLAGS)

AM_CPPFLAGS = -DOOP=1 -DINTERVIEWS=1 -DHOC=1 $(includes)
AM_CFLAGS = @MINGW_CFLAG@

RanFiles = isaac64.c mcran4.c nrnisaac.c nrnran123.c

if BUILD_NRNMPI_DYNAMIC
nrnmpila =
nrnmpidynam = nrnmpi_dynam.c
else
nrnmpila = ../nrnmpi/libnrnmpi.la
nrnmpidynam =
endif

liboc_la_SOURCES = parse.y bksub.c getelm.c lineq.c subrows.c prmat.c \
	code2.c debug.c ocerf.c fileio.c ftime.c \
	getsym.c hoc.c hocedit.c math.c nonlin.c \
	list.c regexp.c audit.c symbol.c \
	version.c xred.c parallel.c functabl.c plot.c plt.c axis.c \
	settext.c x.c fmenu.c termio.c isoc99.c $(nrnmpidynam)\
	scoprand.c $(RanFiles) cygwinprt.c nrnfilewrap.c

## MAC OS X does not allow multiple definitions in separate dyld files
## therefore we create a separate library for the oc objects that
## should appear in oc and ivoc but not nrnoc and nrniv
## modlreg.c is another special case since it should be missing only
## in a special so we will put it explicitly into nrnoc and nrniv
libocxt_la_SOURCES = code.c hoc_init.c hoc_oop.c \
	spinit.c spinit1.c spinit2.c

AM_YFLAGS = -d

EXTRA_DIST = mk_hocusr_h.py hocusr.c macprt.c mswinprt.c system.c \
	$(nrnmpidynam)

if BUILD_MINGW
liboc_la_LIBADD = -lpthread
else
liboc_la_LIBADD = $(X_LIBS)
endif

bin_PROGRAMS = oc

## modlreg.c is here because the newer libtool no longer
## always makes a .o link to the .lo file
oc_SOURCES = ocmain.c ocnoiv.c nocable.c modlreg.c ockludge.c

oc_LDADD = libocxt.la liboc.la \
	$(nrnmpila) \
	@MEMACSLIBLA@ @NRN_READLINE_LIBS@

## hocdec now primary and hoc.h includes it
##hocdec.h: hoc.h hoc2dec.sh
##	./hoc2dec.sh
	
## Header files that need to be installed:
pkginclude_HEADERS = hoc.h hoclist.h hocdec.h hocassrt.h nrnisaac.h \
	oc_ansi.h hocparse.h ocfunc.h ocmisc.h nrnjava.h hocgetsym.h \
	nrnpthread.h hoc_membf.h nrnran123.h scoplib_ansi.h nrnapi.h

## Header files that need to be copied into the distribution but do not need
## to be installed:
noinst_HEADERS = code.h equation.h hocmodl.h hocstr.h \
	profile.h redef.h parse.h mcran4.h nrnmpi.h nrnrt.h isaac64.h \
	nrnfilewrap.h nrn_vsscanf.c classreg.h

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


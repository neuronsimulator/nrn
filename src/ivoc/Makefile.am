lib_LTLIBRARIES = libivoc.la

bin_PROGRAMS = ivoc

DEFS = -Dmotif_kit -Dsgi_motif_kit -Dopenlook_kit -Dbw_kit \
	-Ddefault_kit=SMFKit -DUSEGNU=1 -DUSEMATRIX

# The interviews libraries must be before the X libraries so that if a
# vanilla interviews is also installed, we don't use it.
IV_INCLUDES = @IV_INCLUDE@ $(X_CFLAGS)
nsrc=$(top_srcdir)/src
includes = -I$(nsrc)/gnu -I$(nsrc)/mesch -I$(top_builddir)/src/oc -I$(nsrc)/oc -I$(nsrc)/nrnmpi $(IV_INCLUDES)

IV_LIBS = @IV_LIBS_LIBTOOL@
IVOS_DIR = @IVOS_DIR@
AM_CXXFLAGS = @MINGW_CFLAG@ $(includes)

# sometime multiple definitions are an error
nrniv_iv_sources = classreg.cpp datapath.cpp ocjump.cpp symdir.cpp

cygwin_sources = ivocwin.cpp
x_sources = xdep.cpp field.cpp
carbon_sources = ivocmac.cpp

if BUILD_NRNMPI_DYNAMIC
nrnmpila =
else
nrnmpila = ../nrnmpi/libnrnmpi.la
endif

if BUILD_CYGWIN
sysdep_sources = $(cygwin_sources)
nonsysdep_sources = $(x_sources) $(carbon_sources)
ivoc_LDFLAGS = -mwindows -e _mainCRTStartup
else
if BUILD_MINGW
sysdep_sources = $(cygwin_sources)
nonsysdep_sources = $(x_sources) $(carbon_sources)
ivoc_LDFLAGS =
else
if BUILD_CARBON
sysdep_sources = $(carbon_sources)
nonsysdep_sources = $(x_sources) $(cygwin_sources)
else
sysdep_sources = $(x_sources)
nonsysdep_sources = $(cygwin_sources) $(carbon_sources)
ivoc_LDFLAGS =
endif
endif
endif

libivoc_la_SOURCES = ivoc.cpp xmenu.cpp graph.cpp rubband.cpp grmanip.cpp \
	apwindow.cpp pwman.cpp scene.cpp xyview.cpp rect.cpp mymath.cpp \
	axis.cpp utility.cpp idraw.cpp symchoos.cpp htlist.cpp \
	ocpicker.cpp scenepic.cpp oclist.cpp \
	ocbrowsr.cpp objcmd.cpp ocbox.cpp ocfile.cpp hocmark.cpp oc2iv.cpp \
	epsprint.cpp ivocvect.cpp ivocrand.cpp \
	octimer.cpp ocdeck.cpp bndedval.cpp ochelp.cpp checkpnt.cpp \
	graphvec.cpp strfun.cpp ocobserv.cpp fourier.cpp \
	cbwidget.cpp matrix.cpp ocmatrix.cpp \
	ocpointer.cpp gifimage.cpp ocnoiv1.cpp grglyph.cpp mlinedit.cpp \
	$(sysdep_sources) ivocman1.cpp ocptrvector.cpp

noinst_HEADERS = apwindow.h axis.h bndedval.h cbwidget.h checkpnt.h \
	datapath.h dbrowser.h epsprint.h field.h fourier.h \
	graph.h hocmark.h idraw.h ivoc.h ivocconf.h mymath.h htlist.h \
	objcmd.h oc2iv.h ocbox.h ocbrowsr.h occlass.h ocdeck.h ocfile.h \
	ocglyph.h ocinput.h ocjump.h oclist.h ocmatrix.h ocobserv.h \
	ocpicker.h ocpointer.h random1.h rect.h rubband.h scenepic.h \
	scenevie.h symchoos.h symdir.h utility.h ivocvect.h xmenu.h \
	nrngsl.h nrngsl_hc_radix2.c nrngsl_real_radix2.c \
	grglyph.h nrnmutdec.h ocnotify.h ocptrvector.h

ivoc_SOURCES = nrnmain.cpp ivocmain.cpp $(nrniv_iv_sources)

ivoc_LDADD = ../oc/nocable.o ../oc/modlreg.o \
	../oc/libocxt.la ../oc/liboc.la \
	libivoc.la $(nrnmpila) ../oc/liboc.la \
	@MEMACSLIBLA@ ../mesch/libmeschach.la \
	../gnu/libneuron_gnu.la $(IVOS_DIR) \
	$(IV_LIBS) @NRN_READLINE_LIBS@ @NRNJAVA_LIBLA@ @PTHREAD_LIBS@

## We have to play a trick on automake to get it to install the .o files in
## an architecture-dependent subdirectory.  (Apparently automake's authors
## never heard of installing .o files.)  We pretend to create a separate
## directory (which is really just $(pkglibdir)) and we install the .o files
## by calling them "DATA".
## Argh!!!!!
libobjdir = $(libdir)

## The following object files need to be installed:
libobj_DATA = nrnmain.o ivocmain.o

EXTRA_DIST = macmain.cpp $(nonsysdep_sources)

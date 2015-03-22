## The list of libraries that we make from this directory.
lib_LTLIBRARIES = libnrniv.la

## The list of programs that we make from this directory.
bin_PROGRAMS = nrniv
nsrc=$(top_srcdir)/src

PARSRC = nvector_nrnparallel_ld.c
PARINC = nvector_nrnparallel_ld.h

if BUILD_PARANEURON
PARSRC1 = $(PARSRC)
PARINC1 = $(PARINC)
else
PARSRC1 =
PARINC1 =
endif
EXTRA_DIST = $(PARSRC) $(PARINC) have2want.cpp

if BUILD_NRNMPI_DYNAMIC
nrnmpila =
else
nrnmpila = ../nrnmpi/libnrnmpi.la    
endif

if BUILD_CYGWIN
nrniv_LDFLAGS = -mwindows -e _mainCRTStartup
windll = -I$(nsrc)/mswin/windll
cygexe = mos2nrn.exe neuron.exe
CXX64WIN = x86_64-w64-mingw32-g++
CXX32WIN = i686-pc-mingw32-g++

mos2nrn.exe: $(nsrc)/mswin/extra/mos2nrn.cpp
	if test "$(host_cpu)" = "x86_64" ; then \
	$(CXX64WIN) -g -O2 -DCYGWIN -o mos2nrn.exe $(nsrc)/mswin/extra/mos2nrn.cpp ;\
	else \
	$(CXX32WIN) -g -O2 -DCYGWIN -o mos2nrn.exe $(nsrc)/mswin/extra/mos2nrn.cpp ;\
	fi

neuron.exe: $(nsrc)/mswin/extra/neuron.cpp
	echo "id ICON \"$(nsrc)/mswin/nrniv10.ico\"" > nrnicon.rc
	windres nrnicon.rc -O coff -o nrnicon.res
	if test "$(host_cpu)" = "x86_64" ; then \
	$(CXX64WIN) -g -O2 -DCYGWIN -o neuron.exe $(nsrc)/mswin/extra/neuron.cpp nrnicon.res ;\
	else \
	$(CXX32WIN) -g -O2 -DCYGWIN -o neuron.exe $(nsrc)/mswin/extra/neuron.cpp nrnicon.res ;\
	fi

else
if BUILD_MINGW
nrniv_LDFLAGS =
windll = -I$(nsrc)/mswin/windll
cygexe = mos2nrn.exe neuron.exe

mos2nrn.exe: $(nsrc)/mswin/extra/mos2nrn.cpp
	$(CXX) -g -O2 -DMINGW -o mos2nrn.exe $(nsrc)/mswin/extra/mos2nrn.cpp

neuron.exe: $(nsrc)/mswin/extra/neuron.cpp
	echo "id ICON \"$(nsrc)/mswin/nrniv10.ico\"" > nrnicon.rc
	windres nrnicon.rc -O coff -o nrnicon.res
	$(CXX) -g -O2 -DMINGW -o neuron.exe $(nsrc)/mswin/extra/neuron.cpp nrnicon.res

else
nrniv_LDFLAGS =
windll = 
cygexe =
endif
endif

## These are all the .c files that need to be compiled to make libnrniv.la.

libnrniv_la_SOURCES = nrnmenu.cpp shape.cpp classreg.cpp rotate3d.cpp \
	datapath.cpp symdir.cpp spaceplt.cpp shapeplt.cpp ppshape.cpp \
	secbrows.cpp ndatclas.cpp impedanc.cpp savstate.cpp hocmech.cpp \
	ocjump.cpp vrecord.cpp cvodestb.cpp occvode.cpp \
	cvodeobj.cpp cvtrset.cpp tqueue.cpp netcvode.cpp singlech.cpp \
	nrndaspk.cpp glinerec.cpp cxprop.cpp \
	ocbbs.cpp bbs.cpp bbslocal.cpp bbslsrv.cpp \
	bbsrcli.cpp bbsdirect.cpp bbslsrv2.cpp bbssrv.cpp \
	linmod.cpp linmod1.cpp cachevec.cpp \
	kschan.cpp kssingle.cpp nonlinz.cpp finithnd.cpp nrnste.cpp \
	nrnrtime.cpp nvector_nrnthread.c nrnpy.cpp prcellstate.cpp \
	nvector_nrnthread_ld.c nvector_nrnserial_ld.c \
	$(PARSRC1) bgpmeminfo.c \
	netpar.cpp partrans.cpp splitcell.cpp multisplit.cpp \
	nrndae.cpp matrixmap.cpp geometry3d.cpp

libnrniv_la_LIBADD = @MUSIC_LIBLA@

# The interviews includes must be before the X includes so that if a
# vanilla interviews is also installed, we don't use it.
IV_INCLUDES = @IV_INCLUDE@ $(X_CFLAGS)
IV_LIBS = @IV_LIBS_LIBTOOL@
IVOS_DIR = @IVOS_DIR@

PVM_XTRA_LIBS = @PVM_XTRA_LIBS@
PVM_LIBS = @PVM_LIBS@ $(PVM_XTRA_LIBS)
BBS_LIBS = $(PVM_LIBS)
PVM_INCLUDES = @PVM_INCLUDES@
BBS_INCLUDES = -I$(nsrc)/parallel -I$(nsrc)/nrnmpi $(PVM_INCLUDES) -I@MUSIC_INCDIR@

PTHREAD_CC=@PTHREAD_CC@
PTHREAD_CFLAGS=@PTHREAD_CFLAGS@
PTHREAD_LIBS=@PTHREAD_LIBS@

includes = $(windll) -I$(nsrc)/sundials -I$(nsrc)/sundials/shared -I$(nsrc)/nrncvode -I$(nsrc)/sparse13 -I$(nsrc)/gnu -I. -I$(nsrc)/ivoc -I$(nsrc)/nrnoc -I$(top_builddir)/src/oc -I$(nsrc)/oc $(IV_INCLUDES) $(BBS_INCLUDES)
AM_CPPFLAGS = -DOOP=1 -DCABLE=1 -DUSECVODE=1 -DUSEMATRIX=1 -DUSEBBS=1 $(includes)
AM_CFLAGS = @MINGW_CFLAG@ $(PTHREAD_CFLAGS)
AM_CXXFLAGS = @MINGW_CFLAG@ $(PTHREAD_CFLAGS)

if NRNMECH_DLL_STYLE
libnrniv_la_LDFLAGS = -export-dynamic
else
libnrniv_la_LDFLAGS =
endif

## Sources that go into nrniv:
nrniv_SOURCES = nvkludge.cpp

## Additional libraries and objects that go into nrniv:
nrniv_LDADD = ../ivoc/nrnmain.o ../ivoc/ivocmain.o ../oc/modlreg.o \
	../oc/ockludge.o \
	../nrnoc/libnrnoc.la ../oc/liboc.la \
	libnrniv.la ../ivoc/libivoc.la ../oc/liboc.la \
	$(nrnmpila) \
	../gnu/libneuron_gnu.la \
	../scopmath/libscopmath.la \
	../sparse13/libsparse13.la \
	../sundials/libsundials.la \
	@MEMACSLIBLA@ ../mesch/libmeschach.la $(IVOS_DIR) \
	$(IV_LIBS) @NRNJAVA_LIBLA@ \
	$(BBS_LIBS) @NRNPYTHON_LIBLA@ @MUSIC_LIBLA@ @NRNNI_LIBLA@ \
	@NRN_READLINE_LIBS@ @PTHREAD_LIBS@

nrniv_DEPENDENCIES = \
	../ivoc/nrnmain.o ../ivoc/ivocmain.o ../oc/modlreg.o \
	../oc/ockludge.o \
	../nrnoc/libnrnoc.la ../oc/liboc.la \
	libnrniv.la ../ivoc/libivoc.la \
	$(nrnmpila) \
	../gnu/libneuron_gnu.la \
	../scopmath/libscopmath.la \
	../sparse13/libsparse13.la \
	../sundials/libsundials.la \
	@MEMACSLIBLA@ ../mesch/libmeschach.la \
	@NRN_READLINE_DEP@ @NRNJAVA_DEP@ @NRNPYTHON_DEP@ @NRNNI_DEP@ \
	$(cygexe)


## Header files that must be included in the distribution:
noinst_HEADERS = ndatclas.h nrnclass.h nrnmenu.h nrnoc2iv.h ppshape.h \
	rot3band.h rotate3d.h secbrows.h shape.h shapeplt.h singlech.h \
	linmod.h nrnste.h arraypool.h structpool.h \
	kschan.h kssingle.h nonlinz.h nrnhash.h \
	nvector_nrnthread_ld.h nvector_nrnserial_ld.h \
	$(PARINC1) \
	bgpdma.cpp bgpdmasetup.cpp bgpmeminfo.c \
	multisplitcontrol.h nvector_nrnthread.h \
	nrnmusic.cpp nrndae.h matrixmap.h \
	structpool.h nrnhash_alt.h

## We have to play a trick on automake to get it to install the .o files in
## an architecture-dependent subdirectory.  (Apparently automake's authors
## never heard of installing .o files.)  We pretend to create a separate
## directory (which is really just $(pkglibdir)) and we install the .o files
## by calling them "DATA".
## Argh!!!!!
libobjdir = $(libdir)

## The following object files need to be installed:
libobj_DATA = nvkludge.o


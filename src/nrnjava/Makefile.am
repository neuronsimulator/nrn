SUBDIRS = neuron nrnccmb

## The list of libraries that we make from this directory.
lib_LTLIBRARIES = libnrnjava.la

## These are all the .c files that need to be compiled to make libnrnjava.la.
libnrnjava_la_SOURCES = njvm.cpp nrnjava.cpp nrnjni.cpp

pkginclude_HEADERS = njvm.h

nrnjavaclassdir = $(prefix)/share/@PACKAGE@/classes
nrnjavaclass_DATA = nrnclsld.jar neuron.jar # nrnccmb.jar

JAVAC=@JAVAC@
JAVA=@JAVA@
JAVAH=@JAVAH@

# Flags for compiling C++ code with JNI headers
JNIFLAGS = @JNI_INCLUDE_FLAGS@

# The interviews includes must be before the X includes so that if a
# vanilla interviews is also installed, we don't use it.
IV_INCLUDES = @IV_INCLUDE@ $(X_CFLAGS)

S=$(srcdir)
nsrc=$(top_srcdir)/src
includes = -I$(nsrc)/nrniv -I$(nsrc)/CVODE -I$(nsrc)/nrncvode -I$(nsrc)/sparse13 -I$(nsrc)/gnu \
	-I. -I$(nsrc)/ivoc -I$(nsrc)/nrnoc -I../oc -I$(nsrc)/oc  $(IV_INCLUDES)

AM_CPPFLAGS = -DOOP=1 -DCABLE=1 -DUSECVODE=1 -DUSEMATRIX=1 -DUSEBBS=1 $(JNIFLAGS) $(includes)

if BUILD_NRNJAVA

EXTRA_DIST = neuron.jar nrnclsld.jar nrnccmb.jar \
	mk_njreg.sh njreg.h njredirreg.h neuron_Neuron.h neuron_Redirect.h \
	nrnjava.html javanrn.html

neuron_Neuron.h: neuron/Neuron.class
	$(JAVAH) -classpath $(srcdir) -jni neuron.Neuron

neuron_Redirect.h: neuron/Redirect.class
	$(JAVAH) -classpath $(srcdir) -jni neuron.Redirect

nrnjni.lo nrnjni.o: njreg.h njredirreg.h

njreg.h: neuron_Neuron.h
	$(srcdir)/mk_njreg.sh neuron_Neuron.h njreg

njredirreg.h: neuron_Redirect.h
	$(srcdir)/mk_njreg.sh neuron_Redirect.h njredirreg

nrnjava.lo: neuron_Neuron.h neuron_Redirect.h

nrnclsld.jar: neuron/NrnClassLoader.class
	(cd $(srcdir) ; \
	jar cf nrnclsld.jar neuron/NrnClassLoader.class \
	)

neuron.jar: $(srcdir)/neuron/*.class
	( cd $(srcdir) ; \
	mv neuron/NrnClassLoader.class NrnClassLoader.class ; \
	jar cf neuron.jar neuron/*.class ; \
	mv NrnClassLoader.class neuron/NrnClassLoader.class ; \
	)
else
EXTRA_DIST = mk_njreg.sh nrnjava.html javanrn.html
endif

doc: $(javaSrc)
	javadoc -d doc $(javaSrc)


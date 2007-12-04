lib_LTLIBRARIES = libnrnpython.la

nsrc=$(top_srcdir)/src
DEFS = @DEFS@ @NRNPYTHON_DEFINES@
NRNPYTHON_INCLUDES = @NRNPYTHON_INCLUDES@
IV_INCLUDES = @IV_INCLUDE@ $(X_CFLAGS)
INCLUDES =  -I. -I$(nsrc)/nrniv -I$(nsrc)/ivoc -I$(nsrc)/nrnoc -I$(nsrc)/oc -I$(nsrc)/gnu $(NRNPYTHON_INCLUDES) $(IV_INCLUDES)
AM_CPPFLAGS = -DOOP=1 -DCABLE=1 -DUSECVODE=1 -DUSEMATRIX=1 -DUSEBBS=1
AM_CXXFLAGS = @MINGW_CFLAG@

if BUILD_CYGWIN
if BUILD_NRNPYTHON
EXTEND = inithoc.cpp
else
EXTEND =
endif
else
EXTEND =
endif

libnrnpython_la_SOURCES = nrnpython.cpp nrnpy_hoc.cpp nrnpy_nrn.cpp \
	nrnpy_p2h.cpp $(EXTEND)

noinst_HEADERS = nrnpython.h nrnpy_reg.h
EXTRA_DIST = inithoc.cpp neuron/__init__.py

if BUILD_NRNPYTHON
pysrc = __init__.py
BUILT_SOURCES = force setup.py
force:
	if test inithoc.cpp -ot $(nsrc)/nrnpython/inithoc.cpp ; then \
		cp $(nsrc)/nrnpython/inithoc.cpp inithoc.cpp ; \
	fi
	if test ! -d neuron ; then \
		mkdir neuron ; \
	fi
	for i in $(pysrc) ; do \
		if test neuron/$$i -ot $(nsrc)/nrnpython/neuron/$$i ; then \
			cp $(nsrc)/nrnpython/neuron/$$i neuron/$$i ; \
		fi \
	done
endif

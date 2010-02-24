
CYTHON = @CYTHON@
PYTHON = @PYTHON@
nmsrc = $(top_srcdir)/src/neuronmusic
CXX = @CXX@
INCLUDES = @MUSIC_INCLUDE@ @NRNPYTHON_INCLUDES@ -I.

all: neuronmusic.cpp

neuronmusic.cpp: neuronmusic.pyx neuronmusic.pxd nrnmusic.h
	$(CYTHON) $(nmsrc)/neuronmusic.pyx -o neuronmusic.cpp

EXTRA_DIST = neuronmusic.cpp neuronmusic.pxd nrnmusic.h neuronmusic.pyx

install: neuronmusic.cpp
	if test ! -f neuronmusic.cpp ; then \
		cp $(nmsrc)/neuronmusic.cpp  . ; \
	fi
	$(PYTHON) setup.py install --install-lib=@prefix@/share/nrn/lib/python


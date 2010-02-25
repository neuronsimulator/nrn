
CYTHON = @CYTHON@
PYTHON = @PYTHON@
nmsrc = $(top_srcdir)/src/neuronmusic

all: neuronmusic.cpp

neuronmusic.cpp: neuronmusic.pyx neuronmusic.pxd nrnmusic.h
	$(CYTHON) $(nmsrc)/neuronmusic.pyx -o neuronmusic.cpp

pkginclude_HEADERS = nrnmusic.h
EXTRA_DIST = neuronmusic.cpp neuronmusic.pxd neuronmusic.pyx

install-exec-am: neuronmusic.cpp
	if test ! -f neuronmusic.cpp ; then \
		cp $(nmsrc)/neuronmusic.cpp  . ; \
	fi
	$(PYTHON) setup.py install --install-lib=@prefix@/share/nrn/lib/python

clean-generic:
	rm -r -f build
	if test "$(CYTHON)" != "echo" ; then \
		rm -f neuronmusic.cpp ; \
	fi

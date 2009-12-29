neuronhomedir = $(prefix)/share/@PACKAGE@/lib
neuronhomepythondir = $(neuronhomedir)/python

NRNPYTHON_PYMAJOR = @NRNPYTHON_PYMAJOR@
PY2TO3 = @PY2TO3@

nobase_neuronhomepython_DATA = rdxml.py \
\
neuron/doc.py \
neuron/gui.py \
neuron/__init__.py \
neuron/sections.py \
\
neuron/tests/__init__.py \
neuron/tests/test_all.py \
neuron/tests/test_vector.py


EXTRA_DIST = $(nobase_neuronhomepython_DATA)

install-data-hook:
	if test $(NRNPYTHON_PYMAJOR) -gt 2 ; then \
		$(PY2TO3) -w $(neuronhomepythondir) ; \
	fi


neuronhomedir = $(prefix)/share/@PACKAGE@/lib
neuronhomepythondir = $(neuronhomedir)/python

NRNPYTHON_PYMAJOR = @NRNPYTHON_PYMAJOR@
PY2TO3 = @PY2TO3@

nobase_neuronhomepython_DATA = \
\
neuron/doc.py \
neuron/gui.py \
neuron/__init__.py \
neuron/sections.py \
\
neuron/nonvint_block_supervisor.py \
\
neuron/neuroml/__init__.py \
neuron/neuroml/rdxml.py \
neuron/neuroml/xml2nrn.py \
neuron/neuroml/neuroml.py \
neuron/neuroml/metadata.py \
neuron/neuroml/morphml.py \
neuron/neuroml/biophysics.py \
\
neuron/tests/__init__.py \
neuron/tests/test_all.py \
neuron/tests/test_vector.py \
neuron/rxd/region.py \
neuron/rxd/species.py \
neuron/rxd/rxd.py \
neuron/rxd/reaction.py \
neuron/rxd/rxdsection.py \
neuron/rxd/rxdmath.py \
neuron/rxd/section1d.py \
neuron/rxd/geometry.py \
neuron/rxd/rate.py \
neuron/rxd/node.py \
neuron/rxd/geometry3d/graphicsPrimitives.py \
neuron/rxd/geometry3d/surface.py \
neuron/rxd/geometry3d/surfaces.py \
neuron/rxd/geometry3d/voxelize.py \
neuron/rxd/geometry3d/ctng.py \
neuron/rxd/geometry3d/__init__.py \
neuron/rxd/geometry3d/triangularMesh.py \
neuron/rxd/geometry3d/scalarField.py \
neuron/rxd/__init__.py \
neuron/rxd/multiCompartmentReaction.py \
neuron/rxd/nodelist.py \
neuron/rxd/generalizedReaction.py \
neuron/rxd/morphology.py \
neuron/rxd/options.py \
\
neuron/help_data.dat



EXTRA_DIST = $(nobase_neuronhomepython_DATA)

install-data-hook:
	if test $(NRNPYTHON_PYMAJOR) -gt 2 ; then \
		$(PY2TO3) -w $(DESTDIR)$(neuronhomepythondir) ; \
	fi


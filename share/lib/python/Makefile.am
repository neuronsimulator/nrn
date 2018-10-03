neuronhomedir = $(prefix)/share/@PACKAGE@/lib
neuronhomepythondir = $(neuronhomedir)/python

NRNPYTHON_PYMAJOR = @NRNPYTHON_PYMAJOR@
PY2TO3 = @PY2TO3@

nobase_neuronhomepython_DATA = \
\
neuron/doc.py \
neuron/gui.py \
neuron/hclass2.py \
neuron/hclass3.py \
neuron/__init__.py \
neuron/psection.py \
neuron/sections.py \
\
neuron/gui2/__init__.py \
neuron/gui2/plotshape.py \
neuron/gui2/setup_threejs.py \
neuron/gui2/config.py \
neuron/gui2/rangevar.py \
neuron/gui2/utilities.py \
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
neuron/tests/test_neuron.py \
neuron/tests/test_rxd.py \
neuron/tests/_subclass.py \
\
neuron/crxd/region.py \
neuron/crxd/species.py \
neuron/crxd/rxd.py \
neuron/crxd/reaction.py \
neuron/crxd/rxdsection.py \
neuron/crxd/rxdmath.py \
neuron/crxd/section1d.py \
neuron/crxd/geometry.py \
neuron/crxd/rate.py \
neuron/crxd/node.py \
neuron/crxd/rxdException.py \
neuron/crxd/__init__.py \
neuron/crxd/multiCompartmentReaction.py \
neuron/crxd/nodelist.py \
neuron/crxd/generalizedReaction.py \
neuron/crxd/options.py \
neuron/crxd/gui.py \
neuron/crxd/plugins.py \
neuron/crxd/rangevar.py \
neuron/crxd/dimension3.py \
neuron/crxd/initializer.py \
neuron/crxd/geometry3d/*.* \
\
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
neuron/rxd/rxdException.py \
neuron/rxd/__init__.py \
neuron/rxd/multiCompartmentReaction.py \
neuron/rxd/nodelist.py \
neuron/rxd/generalizedReaction.py \
neuron/rxd/morphology.py \
neuron/rxd/options.py \
neuron/rxd/gui.py \
neuron/rxd/plugins.py \
neuron/rxd/rangevar.py \
neuron/rxd/dimension3.py \
neuron/rxd/initializer.py \
neuron/rxd/geometry3d/*.* \
\
neuron/help_data.dat

SUBDIRS = neuron

EXTRA_DIST = $(nobase_neuronhomepython_DATA)

#install-data-hook:
#	if test $(NRNPYTHON_PYMAJOR) -gt 2 ; then \
#		$(PY2TO3) -w $(DESTDIR)$(neuronhomepythondir) ; \
#	fi


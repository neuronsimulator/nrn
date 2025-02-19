# see PR #3075
# Don't look for KSChan channel __doc__ string with ModelView.

from neuron import h, gui

h.load_file(h.neuronhome() + "/demo/dynclamp.hoc")
h.load_file("mview.hoc")
h.mview()

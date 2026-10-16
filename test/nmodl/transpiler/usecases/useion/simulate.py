import numpy as np

from neuron import h, gui
from neuron.units import ms

nseg = 1

s = h.Section()
s.insert("ionic")
s.nseg = nseg

ena_hoc = h.Vector().record(s(0.5)._ref_ena)
t_hoc = h.Vector().record(h._ref_t)

h.stdinit()
h.tstop = 5.0 * ms
h.run()

ena = np.array(ena_hoc.as_numpy())
t = np.array(t_hoc.as_numpy())

ena_exact = np.full(t.shape, 42.0)
ena_exact[0] = 0.0

abs_err = np.abs(ena - ena_exact)
assert np.all(abs_err < 1e-12), abs_err

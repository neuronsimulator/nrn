import numpy as np

from neuron import h, gui
from neuron.units import ms

nseg = 1

s = h.Section()
s.insert("ionic")
s.nseg = nseg

x_hoc = h.Vector().record(s(0.5)._ref_ena)
t_hoc = h.Vector().record(h._ref_t)

h.stdinit()
h.tstop = 5.0 * ms
h.run()

x = np.array(x_hoc.as_numpy())
t = np.array(t_hoc.as_numpy())

x_exact = np.full(t.shape, 42.0)
x_exact[0] = 0.0

abs_err = np.abs(x - x_exact)

assert np.all(abs_err < 1e-12), abs_err
print("ionic: success")

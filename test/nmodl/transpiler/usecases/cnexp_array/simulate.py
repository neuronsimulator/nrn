import numpy as np

from neuron import h, gui
from neuron.units import ms

nseg = 1

s = h.Section()
s.insert("leonhard")
s.nseg = nseg

x_hoc = h.Vector().record(s(0.5)._ref_x_leonhard)
t_hoc = h.Vector().record(h._ref_t)

h.stdinit()
h.tstop = 5.0 * ms
h.run()

x = np.array(x_hoc.as_numpy())
t = np.array(t_hoc.as_numpy())

rate = (0.1 - 1.0) * (0.7 * 0.8 * 0.9)
x_exact = 42.0 * np.exp(rate*t)
rel_err = np.abs(x - x_exact) / x_exact

assert np.all(rel_err < 1e-12)
print("leonhard: success")

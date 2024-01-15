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
h.tstop = 1.0 * ms
h.run()

x = np.array(x_hoc.as_numpy())
t = np.array(t_hoc.as_numpy())

x_exact = 2.0 * np.ones_like(t)
x_exact[0] = 42;
abs_err = np.abs(x - x_exact)

assert np.all(abs_err < 1e-12), f"{abs_err=}"
print("leonhard: success")

import numpy as np

from neuron import h, gui
from neuron.units import ms

nseg = 1

s = h.Section()
s.nseg = nseg

toggle = h.toggle(s(0.5))

y_hoc = h.Vector().record(toggle._ref_y)
t_hoc = h.Vector().record(h._ref_t)

h.stdinit()
h.tstop = 5.0 * ms
h.run()

t = np.array(t_hoc.as_numpy())
y = np.array(y_hoc.as_numpy())

y_exact = np.array(t >= 2.0, np.float64)

assert np.all(np.abs(y - y_exact) == 0), f"{y} != {y_exact}, delta: {y - y_exact}"

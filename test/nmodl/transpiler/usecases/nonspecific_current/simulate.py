import numpy as np

from neuron import h, gui
from neuron.units import ms

nseg = 1

s = h.Section()
s.insert("leonhard")
s.nseg = nseg

v_hoc = h.Vector().record(s(0.5)._ref_v)
t_hoc = h.Vector().record(h._ref_t)

h.stdinit()
h.tstop = 5.0 * ms
h.run()

v = np.array(v_hoc.as_numpy())
t = np.array(t_hoc.as_numpy())

erev = 1.5
rate = 0.005 / 1e-3
v0 = -65.0
v_exact = erev + (v0 - erev)*np.exp(-rate*t)
rel_err = np.abs(v - v_exact) / np.max(np.abs(v_exact))

assert np.all(rel_err < 1e-1), f"rel_err = {rel_err}"
print("leonhard: success")

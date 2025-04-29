import numpy as np

from neuron import gui, h
from neuron.units import ms


# Test the accuracy of the simulation output to the analytical result
nseg = 1
s = h.Section()
s.insert("leonhard")
s.nseg = nseg
s.c_leonhard = 0.005

v_hoc = h.Vector().record(s(0.5)._ref_v)
t_hoc = h.Vector().record(h._ref_t)

h.stdinit()
h.tstop = 5.0 * ms
h.run()

v = np.array(v_hoc.as_numpy())
t = np.array(t_hoc.as_numpy())

erev = 1.5
rate = s.c_leonhard / 1e-3
v0 = -65.0
v_exact = erev + (v0 - erev) * np.exp(-rate * t)
rel_err = np.abs(v - v_exact) / np.max(np.abs(v_exact))

assert np.all(rel_err < 1e-1), f"rel_err = {rel_err}"


# Test the stability of the simulation at final time using a single timestep
nseg = 1
s = h.Section()
s.insert("leonhard")
s.nseg = nseg
s.c_leonhard = 0.5

v_hoc = h.Vector().record(s(0.5)._ref_v)
t_hoc = h.Vector().record(h._ref_t)

h.stdinit()
h.tstop = 1000.0 * ms
h.dt = 1000.0 * ms
h.run()

v = np.array(v_hoc.as_numpy())
t = np.array(t_hoc.as_numpy())

erev = 1.5
rate = s.c_leonhard / 1e-3
v0 = -65.0
v_exact = erev + (v0 - erev) * np.exp(-rate * t)
rel_err = np.abs(v - v_exact) / np.max(np.abs(v_exact))

assert np.allclose(v[-1], v_exact[-1], atol=0.0), f"rel_err = {rel_err}"


print("leonhard: success")

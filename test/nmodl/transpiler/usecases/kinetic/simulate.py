import numpy as np
import scipy

from neuron import h, gui
from neuron.units import ms

nseg = 1

s = h.Section()
s.insert("X2Y")
s.nseg = nseg

t_hoc = h.Vector().record(h._ref_t)
X_hoc = h.Vector().record(s(0.5)._ref_X_X2Y)
Y_hoc = h.Vector().record(s(0.5)._ref_Y_X2Y)

h.stdinit()
h.tstop = 5.0 * ms
h.run()

t = np.array(t_hoc.as_numpy())
X = np.array(X_hoc.as_numpy())
Y = np.array(Y_hoc.as_numpy())
Z0 = np.array([0, 1])
Z = np.array(list(zip(X, Y)))

A = np.array([[-0.4, 0.5], [0.4, -0.5]])


def f(Z, t):
    return A @ Z


Z_exact = scipy.integrate.odeint(f, Z0, t)

abs_err = np.abs(Z - Z_exact)
assert np.all(abs_err < 0.005), abs_err

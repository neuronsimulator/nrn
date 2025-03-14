import numpy as np

from neuron import h, gui
from neuron.units import ms

# The tests "thread variables". Thread variables are how global variables in
# THREADSAFE code are implemented if they're not read-only. The idea is that
# each thread has it's own copy of the global, with zero synchronization.
#
# The test works be creating two sections, with one segment each. Each section
# is assigned to a different section.
#
# Next, we assign values to `g_w` and use the thread variables to assign to a
# value to `y` (different values at different times during the simulation).

nseg = 1

s0 = h.Section()
s0.insert("threading_effects")
s0.nseg = nseg

s1 = h.Section()
s1.insert("threading_effects")
s1.nseg = nseg

pc = h.ParallelContext()
pc.nthread(2)
pc.partition(0, h.SectionList([s0]))
pc.partition(1, h.SectionList([s1]))

t = h.Vector().record(h._ref_t)
y0 = h.Vector().record(s0(0.5).threading_effects._ref_y)
y1 = h.Vector().record(s1(0.5).threading_effects._ref_y)

# Bunch of arbitrary values:
z0, z1 = 3.0, 4.0
g_w0, g_w1 = 7.0, 2.0
g_w_init = 48.0

# Ensure that the two threads will set different value to `g_w`.
s0(0.5).threading_effects.z = z0
s1(0.5).threading_effects.z = z1

h.g_w_threading_effects = g_w0
assert h.g_w_threading_effects == g_w0
h.stdinit()
assert h.g_w_threading_effects == g_w_init
h.g_w_threading_effects = g_w1
assert h.g_w_threading_effects == g_w1
h.continuerun(1.0 * ms)

# Arguably the value is unspecified, but currently it's the value of
# on thread 0.
assert h.g_w_threading_effects == z0

t = np.array(t.as_numpy())
y0 = np.array(y0.as_numpy())
y1 = np.array(y1.as_numpy())
w = h.g_w_threading_effects

# The solution is piecewise constant. These are the pieces:
i0 = [0]
i1 = np.logical_and(0 < t, t < 0.33)
i2 = np.logical_and(0.33 < t, t < 0.66)
i3 = 0.66 < t

# The values on thread 0:
assert np.all(y0[i0] == g_w_init)
assert np.all(y0[i1] == g_w1)
assert np.all(y0[i2] == 33.3)
assert np.all(y0[i3] == z0 + z0**2)

# The values on thread 1:
assert y1[i0] == g_w_init
# The value of `g_w` on thread 1 is not specified, after setting
# `g_w` from Python.
# assert np.all(y0[i1] == g_w1)
assert np.all(y1[i2] == 34.3)
assert np.all(y1[i3] == z1 + z1**2)

import numpy as np

from neuron import h, gui
from neuron.units import ms

nseg = 1

soma = h.Section()
soma.nseg = nseg
soma.cm = 1.0

syn = h.SnapSyn(soma(0.5))

v0 = -65.0
erev = 10.0
delay = 0.75
dg = 0.8
area = soma(0.5).area()
rate = dg * 1e5 / area

stim = h.NetStim()
stim.interval = 1.0
stim.number = 1
stim.start = delay

netcon = h.NetCon(stim, syn, 0, 0.0, dg)

v_hoc = h.Vector().record(soma(0.5)._ref_v)
t_hoc = h.Vector().record(h._ref_t)

h.dt = 0.01
h.stdinit()
h.tstop = 10.0 * ms
h.run()


def v(t):
    v = np.empty_like(t)
    v[t <= delay] = v0
    v[t >= delay] = erev + (v0 - erev) * np.exp(-rate * (t[t >= delay] - delay))

    return v


t = np.array(t_hoc.as_numpy())
v_approx = np.array(v_hoc.as_numpy())
v_exact = v(t)

assert np.all(np.abs(v_approx - v_exact)) < 1e-10

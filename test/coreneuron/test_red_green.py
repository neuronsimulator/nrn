import numpy as np
import matplotlib.pyplot as plt

from neuron import h, gui
from neuron import coreneuron

enable = True
file_mode = False


h("""create soma""")
h.soma.L = 5.6419
h.soma.diam = 5.6419
h.soma.insert("hh")
h.soma.insert("red")
h.soma.insert("green")
h.soma.nseg = 3

h.tstop = 6.0

# TODO check replay.
# TODO check recording everything and causing a shuffle.

# upsilon is an array variable with at least `m` elements. It's assigned
# the value:
c_red, c_green = 1.0, -1.0

def upsilon(i, c, t):
    return c * np.sin((i + 1.0) * t)

# m = 16
m = 13

t_vector = h.Vector().record(h._ref_t)
upsilon_vector = [
    h.Vector().record(h.soma(0.25)._ref_upsilon_green[0]),
    h.Vector().record(h.soma(0.25)._ref_upsilon_green[m - 1]),
    h.Vector().record(h.soma(0.25)._ref_upsilon_red[0]),
    h.Vector().record(h.soma(0.25)._ref_upsilon_red[m - 1]),
]


if enable:
    pc = h.ParallelContext()

    with coreneuron(enable=enable, file_mode=file_mode):
        h.stdinit()
        pc.psolve(h.tstop)

else:
    h.stdinit()
    h.run()


t = np.array(t_vector.as_numpy())
upsilon_approx = [np.array(u.as_numpy()) for u in upsilon_vector]
upsilon_exact = [
    upsilon(i, c, t)
    for i, c in zip([0, m - 1, 0, m - 1], [-1.0, -1.0, 1.0, 1.0])
]

# for u in upsilon_exact:
#     plt.plot(t, u, "-k", linewidth=2)
#
for u in upsilon_approx:
    plt.plot(t, u, "--", linewidth=2)

plt.show()

for k, (u_exact, u_approx) in enumerate(zip(upsilon_exact, upsilon_approx)):

    assert np.all(np.abs(u_approx - u_exact) < 1e-10)
    assert np.all(np.abs(u_approx - u_exact) < 1e-10)

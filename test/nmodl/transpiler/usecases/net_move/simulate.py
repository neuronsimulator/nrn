import numpy as np

from neuron import h, gui
from neuron.units import ms

# A test that demonstrates the behaviour of `net_move` and
# `flag`.
#
# The `flag == 0` means that an external event was received. Therefore, we send
# 2 external events at times `1.001` and `2.001`. External events increment the
# counter `y`; and cause a self event 0.1 after receiving the external event. A
# self event will cause another self event 2.0 later and increments `z`.
#
# The function `net_move` changes the delivery times of the running/existing
# self event to the new value.
#
# External events happen at: [1.001, 2.001]
# Self events happen at:     [1.101, 2.101, 4.101]
#
# Note, the offsetting by `0.001` implies that the event arrives in the first
# half of the time-step. Therefore, the change is recorded in time for the next
# time-step.


def simulate(create_spiker):
    nseg = 1

    s = h.Section()
    s.nseg = nseg

    stim = h.NetStim()
    stim.interval = 1.0
    stim.number = 2
    stim.start = 0.001

    spiker = create_spiker(s)
    nc = h.NetCon(stim, spiker)

    t_hoc = h.Vector().record(h._ref_t)
    y_hoc = h.Vector().record(spiker._ref_y)
    z_hoc = h.Vector().record(spiker._ref_z)

    h.stdinit()
    h.tstop = 5.0 * ms
    h.run()

    t = np.array(t_hoc.as_numpy())
    y = np.array(y_hoc.as_numpy())
    z = np.array(z_hoc.as_numpy())

    return t, y, z


def check_solution(t, y, z):
    y_exact = np.zeros(t.shape)
    y_exact[t > 1.001] += 1.0
    y_exact[t > 2.001] += 1.0

    z_exact = np.zeros(t.shape)
    z_exact[t > 1.101] += 1
    z_exact[t > 2.101] += 1
    z_exact[t > 4.101] += 1

    assert np.all(
        np.abs(y - y_exact) < 1e-12
    ), f"{y} != {y_exact}, delta: {y - y_exact}"

    assert np.all(
        np.abs(z - z_exact) < 1e-12
    ), f"{z} != {z_exact}, delta: {z - z_exact}"


def plot_solution(t, y, z):
    import matplotlib.pyplot as plt

    plt.plot(t, y)
    plt.plot(t, z)
    plt.show()


if __name__ == "__main__":
    t, y, z = simulate(lambda s: h.art_spiker(s(0.5)))
    check_solution(t, y, z)

    # plot_solution(t, y, z)

    t, y, z = simulate(lambda s: h.spiker(s(0.5)))
    check_solution(t, y, z)

    # plot_solution(t, y, z)

import numpy as np

from neuron import h, gui
from neuron.units import ms

# Send a message from `spiker` to `receiver` using a
# `net_event` in a NET_RECEIVE block.
#
# Since the NET_RECEIVE block in the `spiker` needs to
# be triggered we add a `stim`.


def simulate():
    nseg = 1

    s1 = h.Section()
    s1.nseg = nseg

    s2 = h.Section()
    s2.nseg = nseg

    spiker = h.spiker(s1(0.5))
    receiver = h.receiver(s2(0.5))

    stim = h.NetStim()
    stim.interval = 0.1
    stim.number = 100
    stim.start = 0.01

    nc1 = h.NetCon(stim, spiker, 0, 0.0, 0.0)
    nc2 = h.NetCon(spiker, receiver, 0, 0.0, 0.0)

    t_hoc = h.Vector().record(h._ref_t)
    y_hoc = h.Vector().record(receiver._ref_y)

    h.stdinit()
    h.tstop = 5.0 * ms
    h.run()

    t = np.array(t_hoc.as_numpy())
    y = np.array(y_hoc.as_numpy())

    return t, y


def check_solution(t, y, arrival_times):
    eps = 1e-8
    y_exact = np.zeros(y.shape)
    for t_arrival in arrival_times:
        # Spikes arrive on the next time step after t_arrival.
        y_exact[t > t_arrival + eps] += 0.1

    assert np.all(
        np.abs(y - y_exact) < 1e-12
    ), f"{y} != {y_exact}, delta: {y - y_exact}"


def plot_solution(t, y):
    import matplotlib.pyplot as plt

    plt.plot(t, y)
    plt.show()


if __name__ == "__main__":
    t, y = simulate()
    arrival_times = np.arange(1, 5)
    check_solution(t, y, arrival_times)

    # plot_solution(t, y)

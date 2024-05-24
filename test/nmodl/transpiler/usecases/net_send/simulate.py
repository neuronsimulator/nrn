import numpy as np

from neuron import h, gui
from neuron.units import ms

# In the INITIAL block an event is sent to itself using `net_send`. Once for
# POINT_PROCESS (toggle.mod) and once for ARTIFICIAL_CELL (art_toggle.mod).
#
# When the event is received the variable `y` is "toggled" to 1. This happens
# at different times for `toggle` and `art_toggle`.


def simulate():
    nseg = 1

    s = h.Section()
    s.nseg = nseg

    toggle = h.toggle(s(0.5))
    art_toggle = h.art_toggle()

    t_hoc = h.Vector().record(h._ref_t)
    y_hoc = h.Vector().record(toggle._ref_y)
    art_y_hoc = h.Vector().record(art_toggle._ref_y)

    h.stdinit()
    h.tstop = 5.0 * ms
    h.run()

    t = np.array(t_hoc.as_numpy())
    y = np.array(y_hoc.as_numpy())
    art_y = np.array(art_y_hoc.as_numpy())

    return t, y, art_y


def check_solution(t, y, art_y):
    y_exact = np.array(t >= 2.0, np.float64)
    art_y_exact = np.array(t >= 1.5, np.float64)

    assert np.all(np.abs(y - y_exact) == 0), f"{y} != {y_exact}, delta: {y - y_exact}"
    assert np.all(
        np.abs(art_y - art_y_exact) == 0
    ), f"{art_y} != {art_y_exact}, delta: {art_y - art_y_exact}"


def plot_solution(t, y, art_y):
    import matplotlib.pyplot as plt

    plt.plot(t, y)
    plt.plot(t, art_y)
    plt.show()


if __name__ == "__main__":
    t, y, art_y = simulate()
    check_solution(t, y, art_y)

    # plot_solution(t, y, art_y)

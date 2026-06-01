import numpy as np

from neuron import h, gui
from neuron.units import ms

# In the INITIAL block an event is sent to itself using `net_send`. Once for
# POINT_PROCESS (toggle.mod) and once for ARTIFICIAL_CELL (art_toggle.mod).
#
# When the event is received the variable `y` is "toggled" to 1. This happens
# at different times for `toggle` and `art_toggle`.
#
# To test `net_send` inside a NET_RECEIVE block, after toggling we send another
# event (to ourselves) which will cause `y` to increment once more.

# Recording variables affected by events happens as follows:
# 1. Apply all events in `[t_i, t_i + 0.5 *dt]`.
# 2. Integrate: t_i - 0.5*dt -> t_i + 0.5*dt
# 3. Record variables.
# 4. Apply all events in `[t_i+0.5*dt, t_{i+1}]`.
#
# Therefore, we make sure that the events happen in the first half of a
# time-step.


def simulate(create_toggle):
    nseg = 1

    s = h.Section()
    s.nseg = nseg

    toggle = create_toggle(s)

    t_hoc = h.Vector().record(h._ref_t)
    y_hoc = h.Vector().record(toggle._ref_y)

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
        y_exact[t > t_arrival - eps] += 1

    assert np.all(y == y_exact), f"{y} != {y_exact}, delta = {y - y_exact}"


if __name__ == "__main__":
    t, y = simulate(lambda s: h.toggle(s(0.5)))
    check_solution(t, y, [2.001, 4.001])

    t, y = simulate(lambda s: h.art_toggle(s(0.5)))
    check_solution(t, y, [2.501, 4.501])

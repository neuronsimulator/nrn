import numpy as np
from neuron import gui
from neuron import h


def simulate():
    s = h.Section()
    s.insert("thread_newton")

    t_hoc = h.Vector().record(h._ref_t)
    x_hoc = h.Vector().record(s(0.5).thread_newton._ref_x)

    h.stdinit()
    h.run()

    t = np.array(t_hoc.as_numpy())
    x = h.Vector(x_hoc.as_numpy())

    # The ODE for X is `dX/dt = 42.0`:
    x_exact = 42.0 * t

    assert np.all(np.abs(x - x_exact) < 1e-10)


if __name__ == "__main__":
    simulate()

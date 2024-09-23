import numpy as np
from neuron import h, gui
from neuron.units import ms


def test_finite_difference():
    nseg = 1

    s = h.Section()
    s.insert("finite_difference")
    s.nseg = nseg

    x_hoc = h.Vector().record(s(0.5)._ref_x_finite_difference)
    t_hoc = h.Vector().record(h._ref_t)

    h.stdinit()
    h.dt = 0.001
    h.tstop = 5.0 * ms
    h.run()

    x = np.array(x_hoc.as_numpy())
    t = np.array(t_hoc.as_numpy())

    a = h.a_finite_difference
    x_exact = 42.0 * np.exp(-a * t)
    np.testing.assert_allclose(x, x_exact, rtol=1e-4)


if __name__ == "__main__":
    test_finite_difference()

import numpy as np
from neuron import h, gui
from neuron.units import ms


def simulate(method, rtol, dt=None):
    nseg = 1

    s = h.Section()
    s.insert(f"{method}_array")
    s.nseg = nseg

    x_hoc = h.Vector().record(getattr(s(0.5), f"_ref_x_{method}_array"))
    t_hoc = h.Vector().record(h._ref_t)

    h.stdinit()
    if dt is not None:
        h.dt = dt * ms
    h.tstop = 5.0 * ms
    h.run()

    x = np.array(x_hoc.as_numpy())
    t = np.array(t_hoc.as_numpy())

    rate = (0.1 - 1.0) * (0.7 * 0.8 * 0.9)
    x_exact = 42.0 * np.exp(rate * t)
    np.testing.assert_allclose(x, x_exact, rtol=rtol)


if __name__ == "__main__":
    simulate("cnexp", rtol=1e-12)
    simulate("derivimplicit", rtol=1e-4, dt=1e-4)

from typing import Optional

from scipy.special import lambertw
import numpy as np
from neuron import h, gui
from neuron.units import ms


def test_cnexp_to_derivimplicit(
    mech: str,
    rtol: float,
    dt: Optional[float] = None,
):
    """
    Test that NMODL changes the solver from cnexp to derivimplicit if it
    detects the Lambert W function in the solution
    """
    nseg = 1

    s = h.Section()
    s.insert(mech)
    s.nseg = nseg

    x_hoc = h.Vector().record(getattr(s(0.5), f"_ref_x_{mech}"))
    t_hoc = h.Vector().record(h._ref_t)

    h.stdinit()
    if dt is not None:
        h.dt = dt * ms
    h.tstop = 5.0 * ms
    h.run()

    x = np.array(x_hoc.as_numpy())
    t = np.array(t_hoc.as_numpy())

    x_exact = lambertw(42 * np.exp(-t) * np.exp(42))
    np.testing.assert_allclose(x, x_exact, rtol=rtol)


if __name__ == "__main__":
    test_cnexp_to_derivimplicit("cnexp_to_derivimplicit", rtol=1.1e-6)

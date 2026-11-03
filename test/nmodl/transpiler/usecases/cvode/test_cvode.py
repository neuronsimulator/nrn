import numpy as np
from neuron import gui
from neuron import h
from neuron.units import ms


def test_cvode(rtol):
    nseg = 1
    mech = "scalar"

    s = h.Section()
    cvode = h.CVode()
    cvode.active(True)
    cvode.atol(1e-10)
    s.insert(mech)
    s.nseg = nseg

    t_hoc = h.Vector().record(h._ref_t)
    var1_hoc = h.Vector().record(getattr(s(0.5), f"_ref_var1_{mech}"))
    var2_hoc = h.Vector().record(getattr(s(0.5), f"_ref_var2_{mech}"))
    var3_hoc = h.Vector().record(getattr(s(0.5), f"_ref_var3_{mech}"))

    h.stdinit()
    h.tstop = 2.0 * ms
    h.run()

    freq = getattr(h, f"freq_{mech}")
    a = getattr(h, f"a_{mech}")
    v1 = getattr(h, f"v1_{mech}")
    v2 = getattr(h, f"v2_{mech}")
    v3 = getattr(h, f"v3_{mech}")
    r = getattr(h, f"r_{mech}")
    k = getattr(h, f"k_{mech}")

    t = np.array(t_hoc.as_numpy())
    var1 = np.array(var1_hoc.as_numpy())
    var2 = np.array(var2_hoc.as_numpy())
    var3 = np.array(var3_hoc.as_numpy())

    var1_exact = (np.cos(t * freq) + v1 * freq - 1) / freq
    var2_exact = v2 * np.exp(-t * a)
    var3_exact = k * v3 / (v3 + (k - v3) * np.exp(-r * t))

    np.testing.assert_allclose(var1, var1_exact, rtol=rtol)
    np.testing.assert_allclose(var2, var2_exact, rtol=rtol)
    np.testing.assert_allclose(var3, var3_exact, rtol=rtol)

    return t, var1, var2, var3


if __name__ == "__main__":
    t, *x = test_cvode(rtol=1e-5)

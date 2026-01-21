from neuron import h, gui

import numpy as np


def test_voltage_access():
    s = h.Section()
    s.insert("accessors")

    h.finitialize()
    v = s(0.5).v
    vinst = s(0.5).accessors.get_voltage()
    # The voltage will be consistent right after
    # finitialize.
    assert vinst == v

    for _ in range(4):
        v = s(0.5).v
        h.fadvance()
        vinst = s(0.5).accessors.get_voltage()

        # During timestepping the internal copy
        # of the voltage lags behind the current
        # voltage by some timestep.
        assert vinst == v, f"{vinst = }, {v = }, delta = {vinst - v}"


def check_ode(mech_name, step):
    s = h.Section()
    s.insert(mech_name)

    h.finitialize()

    c = -0.001 / 1e-3

    for _ in range(4):
        v_expected = step(s(0.5).v, c)
        h.fadvance()
        np.testing.assert_approx_equal(s(0.5).v, v_expected, significant=10)


def test_breakpoint():
    # Results in backward Euler.
    check_ode("ode", lambda v, c: (1.0 - c * h.dt) ** (-1.0) * v)


def test_state():
    # Effectively, the timing when states are computed results in backward Euler.
    check_ode("state_ode", lambda v, c: (1.0 + c * h.dt) * v)


if __name__ == "__main__":
    test_voltage_access()
    test_breakpoint()
    test_state()

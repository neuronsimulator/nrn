import numpy as np

from neuron import h, gui
from neuron.units import ms


def run_simulation():
    s = h.Section()
    s.insert("side_effects")

    mech = s(0.5).side_effects
    t_hoc = h.Vector().record(h._ref_t)
    x_hoc = h.Vector().record(mech._ref_x)
    X_hoc = h.Vector().record(mech._ref_X)
    Y_hoc = h.Vector().record(mech._ref_Y)
    forward_flux_hoc = h.Vector().record(mech._ref_forward_flux)
    backward_flux_hoc = h.Vector().record(mech._ref_backward_flux)

    h.stdinit()
    h.tstop = 5.0 * ms
    h.run()

    timeseries = {
        "t": np.array(t_hoc.as_numpy()),
        "x": np.array(x_hoc.as_numpy()),
        "X": np.array(X_hoc.as_numpy()),
        "Y": np.array(Y_hoc.as_numpy()),
        "forward_flux": np.array(forward_flux_hoc.as_numpy()),
        "backward_flux": np.array(backward_flux_hoc.as_numpy()),
    }

    return timeseries


def check_assignment(x, X):
    # At time t = 0, the side effects aren't applied.
    np.testing.assert_array_equal(x[1:], X[1:])


def check_flux(actual_flux, expected_flux):
    # At time t = 0, the side effects aren't applied.
    np.testing.assert_array_almost_equal_nulp(
        actual_flux[1:], expected_flux[1:], nulp=8
    )


def check_forward_flux(X, actual_flux):
    check_flux(actual_flux, 0.4 * X)


def check_backward_flux(Y, actual_flux):
    check_flux(actual_flux, 0.5 * Y)


if __name__ == "__main__":
    timeseries = run_simulation()
    check_assignment(timeseries["x"], timeseries["X"])
    check_forward_flux(timeseries["X"], timeseries["forward_flux"])
    check_backward_flux(timeseries["Y"], timeseries["backward_flux"])

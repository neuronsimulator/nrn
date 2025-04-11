import sys
import pickle

import numpy as np

from neuron import h, gui


def run(steady_state):
    s = h.Section()

    s.insert("minipump")
    s.diam = 1.0

    t_hoc = h.Vector().record(h._ref_t)
    X_hoc = h.Vector().record(s(0.5).minipump._ref_X)
    Y_hoc = h.Vector().record(s(0.5).minipump._ref_Y)
    Z_hoc = h.Vector().record(s(0.5).minipump._ref_Z)

    h.run_steady_state_minipump = 1.0 if steady_state else 0.0

    h.stdinit()
    h.continuerun(1.0)

    t = np.array(t_hoc.as_numpy())
    X = np.array(X_hoc.as_numpy())
    Y = np.array(Y_hoc.as_numpy())
    Z = np.array(Z_hoc.as_numpy())

    return t, X, Y, Z


def traces_filename(steady_state):
    return "test_minipump{}.pkl".format("-steady_state" if steady_state else "")


def save_traces(t, X, Y, Z, steady_state):
    with open(traces_filename(steady_state), "bw") as f:
        pickle.dump({"t": t, "X": X, "Y": Y, "Z": Z}, f)


def load_traces(steady_state):
    with open(traces_filename(steady_state), "br") as f:
        d = pickle.load(f)

    return d["t"], d["X"], d["Y"], d["Z"]


def assert_almost_equal(actual, expected, rtol):
    decimal = np.ceil(-np.log10(rtol * np.max(expected)))
    np.testing.assert_almost_equal(actual, expected, decimal=decimal)


def check_traces(t, X, Y, Z, steady_state):
    if len(sys.argv) < 2:
        return

    codegen = sys.argv[1]
    if codegen == "nocmodl":
        save_traces(t, X, Y, Z, steady_state)

    else:
        t_ref, X_ref, Y_ref, Z_ref = load_traces(steady_state)

        assert_almost_equal(t, t_ref, rtol=1e-8)
        assert_almost_equal(X, X_ref, rtol=1e-8)
        assert_almost_equal(Y, Y_ref, rtol=1e-8)
        assert_almost_equal(Z, Z_ref, rtol=1e-8)


def check_solution(steady_state):
    t, X, Y, Z = run(steady_state)
    check_traces(t, X, Y, Z, steady_state=steady_state)


def test_steady_state():
    check_solution(steady_state=True)


def test_no_steady_state():
    check_solution(steady_state=False)


if __name__ == "__main__":
    test_steady_state()
    test_no_steady_state()

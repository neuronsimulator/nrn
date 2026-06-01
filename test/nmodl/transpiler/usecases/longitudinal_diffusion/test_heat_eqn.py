import os
import sys
import pickle

from neuron import h, gui

import numpy as np


def reference_filename(mech_name):
    return f"diffuse-{mech_name}.pkl"


def save_state(mech_name, t, X):
    with open(reference_filename(mech_name), "wb") as f:
        pickle.dump((t, X), f)


def load_state(mech_name):
    filename = reference_filename(mech_name)
    if not os.path.exists(filename):
        raise RuntimeError("References unavailable. Try running with NOCMODL first.")

    with open(filename, "rb") as f:
        return pickle.load(f)


def simulator_name():
    return sys.argv[1] if len(sys.argv) >= 2 else None


def run_simulation(mech_name, record_states):
    nseg = 50

    s = h.Section()
    s.nseg = nseg
    s.insert(mech_name)

    t_hoc = h.Vector().record(h._ref_t)
    X_hoc = []
    for i in range(nseg):
        x = (0.5 + i) / nseg
        inst = getattr(s(x), mech_name)

        inst.x = x
        X_hoc.append(record_states(inst))

    h.finitialize()
    h.continuerun(1.0)

    t = np.array(t_hoc.as_numpy())
    X = np.array([[np.array(xx.as_numpy()) for xx in x] for x in X_hoc])

    # The axes are:
    #   time, spatial position, state variable
    X = np.transpose(X, axes=(2, 0, 1))

    return t, X


def check_timeseries(mech_name, t, X):
    t_noc, X_noc = load_state(mech_name)

    np.testing.assert_allclose(t, t_noc, atol=1e-10, rtol=0.0)
    np.testing.assert_allclose(X, X_noc, atol=1e-10, rtol=0.0)


def plot_timeseries(mech_name, t, X, i_state):
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        return

    nseg = X.shape[1]
    frames_with_label = [0, 1, len(t) - 1]

    fig = plt.figure()
    for i_time, t in enumerate(t):
        kwargs = {"label": f"t = {t:.3f}"} if i_time in frames_with_label else dict()

        x = [(0.5 + i) / nseg for i in range(nseg)]
        plt.plot(x, X[i_time, :, i_state], **kwargs)

    plt.xlabel("Spatial position")
    plt.ylabel(f"STATE value: X[{i_state}]")
    plt.title(f"Simulator: {simulator_name()}")
    plt.legend()
    plt.savefig(f"diffusion-{mech_name}-{simulator_name()}-state{i_state}.png", dpi=300)
    plt.close(fig)


def check_heat_equation(mech_name, record_states):
    t, X = run_simulation(mech_name, record_states)

    for i_state in range(X.shape[2]):
        plot_timeseries(mech_name, t, X, i_state)

    simulator = sys.argv[1]
    if simulator == "nocmodl":
        save_state(mech_name, t, X)

    else:
        check_timeseries(mech_name, t, X)


def record_states_factory(array_size, get_reference):
    return lambda inst: [
        h.Vector().record(get_reference(inst, k)) for k in range(array_size)
    ]


def check_heat_equation_scalar(mech_name):
    check_heat_equation(
        mech_name, record_states_factory(1, lambda inst, k: inst._ref_X)
    )


def test_heat_equation_scalar():
    check_heat_equation_scalar("heat_eqn_scalar")


def test_heat_equation_global():
    check_heat_equation_scalar("heat_eqn_global")


def test_heat_equation_function():
    check_heat_equation_scalar("heat_eqn_function")


def test_heat_equation_thread_vars():
    check_heat_equation_scalar("heat_eqn_thread_vars")


def test_heat_equation_array():
    check_heat_equation(
        "heat_eqn_array", record_states_factory(4, lambda inst, k: inst._ref_X[k])
    )


if __name__ == "__main__":
    test_heat_equation_scalar()
    test_heat_equation_global()
    test_heat_equation_thread_vars()
    test_heat_equation_function()
    test_heat_equation_array()

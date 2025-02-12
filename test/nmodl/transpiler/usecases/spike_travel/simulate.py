import numpy as np

from neuron import gui, h
from neuron.units import ms


def simulate():
    nseg = 100

    s0 = h.Section()
    s0.nseg = nseg
    s0.L = 300000
    s0.insert("hodhux")
    ic = h.IClamp(s0(0.001))
    ic.delay = 0
    ic.dur = 1e9
    ic.amp = 1000

    s1 = h.Section()
    s1.nseg = nseg
    s1.L = 300000
    s1.insert("hodhux")

    syn = h.ExpSyn2(s1(0.99))

    threshold = 10
    delay = 10
    weight = 1000
    nc = h.NetCon(s0(0.9)._ref_v, syn, threshold, delay, weight)

    xs = 0.2 * np.arange(5) + 0.1

    v0_hoc = [h.Vector().record(s0(x)._ref_v) for x in xs]
    v1_hoc = [h.Vector().record(s1(x)._ref_v) for x in xs]
    t_hoc = h.Vector().record(h._ref_t)

    h.stdinit()
    h.tstop = 100.0 * ms
    h.run()

    v0 = [np.array(vv.as_numpy()) for vv in v0_hoc]
    v1 = [np.array(vv.as_numpy()) for vv in v1_hoc]
    t = np.array(t_hoc.as_numpy())

    return t, xs, v0, v1


def tv(v):
    return np.sum(np.abs(np.diff(v)))


def l1(v):
    return np.sum(np.abs(v)) / v.size


def arg_minmax(t, v):
    tmin = t[np.argmin(v)]
    tmax = t[np.argmax(v)]

    return tmin, tmax


def assert_close(approx, expected, rtol):
    assert (
        np.abs(approx - expected) < rtol * expected
    ), f"{approx=}, {expected=}, delta = {approx - expected}"


def check_single_solution(t, v, ref):
    assert_close(tv(v), ref["tv"], 0.01)
    assert_close(l1(v), ref["l1"], 0.01)

    for approx, expected in zip(arg_minmax(t, v), ref["arg_minmax"]):
        assert_close(approx, expected, 0.1)


def check_solution(t, v0, v1):
    v0_ref = {"tv": 227.68, "l1": 64.606, "arg_minmax": (8.1130, 4.8256)}
    check_single_solution(t, v0, v0_ref)

    v1_ref = {"tv": 227.92, "l1": 64.629, "arg_minmax": (59.6799, 56.3917)}
    check_single_solution(t, v1, v1_ref)


def plot_solution(t, v0, v1):
    import matplotlib.pyplot as plt

    for xx, vv in zip(x, v0):
        plt.plot(t, vv, linewidth=3, label=f"v0({xx:.1f})")

    for xx, vv in zip(x, v1):
        plt.plot(t, vv, "--", linewidth=3, label=f"v1({xx:.1f})")

    plt.legend()
    plt.xlabel("Time")
    plt.ylabel("Voltage")
    plt.show()


if __name__ == "__main__":
    t, x, v0, v1 = simulate()
    check_solution(t, v0[0], v1[0])

    # plot_solution(t, v0, v1)

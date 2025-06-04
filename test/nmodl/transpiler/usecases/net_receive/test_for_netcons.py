import numpy as np

from neuron import h, gui
from neuron.units import ms


def assert_equal(a, b):
    assert a == b, f"{a} != {b}"


def test_for_netcons():
    nseg = 1

    soma = h.Section()
    soma.nseg = nseg

    syn1 = h.ForNetconsSyn(soma(0.5))
    syn1.a0 = 1.0

    syn2 = h.ForNetconsSyn(soma(0.5))
    syn2.a0 = -1.0

    stim = h.NetStim()
    stim.interval = 1.0
    stim.number = 3
    stim.start = 0.2

    nc1 = h.NetCon(stim, syn1, 0, 0.0, 0.1)
    nc2 = h.NetCon(stim, syn2, 0, 0.0, 0.2)
    nc3 = h.NetCon(stim, syn1, 0, 0.0, 0.3)

    all_ncs = [nc1, nc2, nc3]

    h.stdinit()

    for nc in all_ncs:
        # w and a
        assert_equal(nc.wcnt(), 2)

    assert_equal(nc1.weight[0], 0.1)
    assert_equal(nc1.weight[1], 1.0)

    assert_equal(nc2.weight[0], 0.2)
    assert_equal(nc2.weight[1], -1.0)

    assert_equal(nc3.weight[0], 0.3)
    assert_equal(nc3.weight[1], 1.0)

    h.continuerun(10.0 * ms)

    # Each event doubles the value of all connected
    # NetCons.
    assert_equal(nc1.weight[1], 2**6 * syn1.a0)
    assert_equal(nc3.weight[1], 2**6 * syn1.a0)
    assert_equal(nc2.weight[1], 2**3 * syn2.a0)


if __name__ == "__main__":
    test_for_netcons()

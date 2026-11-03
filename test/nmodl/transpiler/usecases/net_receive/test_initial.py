import numpy as np

from neuron import h, gui
from neuron.units import ms


def test_net_receive_initial():
    nseg = 1

    soma = h.Section()
    soma.nseg = nseg

    syn = h.InitSyn(soma(0.5))
    syn.a0 = 329.012

    stim = h.NetStim()
    nc = h.NetCon(stim, syn, 0, 0.0, 0.1)

    h.stdinit()

    assert nc.wcnt() == 2
    assert nc.weight[0] == 0.1
    assert nc.weight[1] == syn.a0


if __name__ == "__main__":
    test_net_receive_initial()

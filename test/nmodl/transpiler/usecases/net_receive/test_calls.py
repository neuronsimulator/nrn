import numpy as np

from neuron import h, gui
from neuron.units import ms


def test_calls():
    nseg = 1

    soma = h.Section()
    soma.nseg = nseg

    syn = h.NetReceiveCalls(soma(0.5))

    stim = h.NetStim()
    stim.interval = 0.2
    stim.number = 3
    stim.start = 0.1

    netcon = h.NetCon(stim, syn)

    h.stdinit()
    h.run()

    assert syn.c1 == 3
    assert syn.c2 == 6


if __name__ == "__main__":
    test_calls()

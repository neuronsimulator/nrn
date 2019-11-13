import pytest
import neuron
from neuron import h

def func(x):
    return x + 1


def test_answer():
    assert func(3) == 4


def test_neuron():
    h('''create soma''')
    h.load_file("stdrun.hoc")
    h.soma.L=5.6419
    h.soma.diam=5.6419
    h.soma.insert("hh")
    ic = h.IClamp(h.soma(.5))
    ic.delay = .5
    ic.dur = 0.1
    ic.amp = 0.3

    v = h.Vector()
    v.record(h.soma(.5)._ref_v, sec = h.soma)
    tv = h.Vector()
    tv.record(h._ref_t, sec=h.soma)
    h.run()
    assert(v[0] == -65.0)

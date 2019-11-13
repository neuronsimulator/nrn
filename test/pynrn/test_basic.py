import pytest
<<<<<<< Updated upstream
from neuron import h, gui
import sys
=======
import neuron
from neuron import h, gui
>>>>>>> Stashed changes

def func(x):
    return x + 1


def test_answer():
    assert func(3) == 4


def test_neuron():
    h('''create soma''')
    h.soma.L=5.6419
    h.soma.diam=5.6419
    h.soma.insert("hh")
    ic = h.IClamp(h.soma(.5))
    ic.delay = .5
    ic.dur = 0.1
    ic.amp = 0.3

<<<<<<< Updated upstream
    h.cvode.use_fast_imem(1)
    h.cvode.cache_efficient(1)

    v = h.Vector()
    v.record(h.soma(.5)._ref_v, sec = h.soma)
    i_mem = h.Vector()
    i_mem.record(h.soma(.5)._ref_i_membrane_, sec = h.soma)
    tv = h.Vector()
    tv.record(h._ref_t, sec=h.soma)
    h.run()
    print(v[1])
=======
    v = h.Vector()
    v.record(h.soma(.5)._ref_v, sec = h.soma)
    tv = h.Vector()
    tv.record(h._ref_t, sec=h.soma)
    h.run()
    assert(v[0] == -65.0)
>>>>>>> Stashed changes

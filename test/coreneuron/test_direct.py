import pytest
import sys

from neuron import h, gui

def test_direct_memory_transfer():
    h('''create soma''')
    h.soma.L=5.6419
    h.soma.diam=5.6419
    h.soma.insert("hh")
    ic = h.IClamp(h.soma(.5))
    ic.delay = .5
    ic.dur = 0.1
    ic.amp = 0.3

    #for testing external mod file
    h.soma.insert("Sample")

    h.cvode.use_fast_imem(1)
    h.cvode.cache_efficient(1)

    v = h.Vector()
    v.record(h.soma(.5)._ref_v, sec = h.soma)
    i_mem = h.Vector()
    i_mem.record(h.soma(.5)._ref_i_membrane_, sec = h.soma)
    tv = h.Vector()
    tv.record(h._ref_t, sec=h.soma)
    h.run()
    vstd = v.cl()
    tvstd = tv.cl()
    i_memstd = i_mem.cl()

    from neuron import coreneuron
    coreneuron.enable = True

    pc = h.ParallelContext()
    h.stdinit()
    pc.psolve(h.tstop)

    assert(tv.eq(tvstd))
    assert(v.cl().sub(vstd).abs().max() < 1e-10)
    assert(i_mem.cl().sub(i_memstd).abs().max() < 1e-10)

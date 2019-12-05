import sys

import numpy as np

from neuron import h, hoc


def test_soma():
    h('''create soma''')

    assert h.soma is not None

    h.soma.L = 5.6419
    h.soma.diam = 5.6419

    assert h.soma.L == 5.6419
    assert h.soma.diam == 5.6419


def test_simple_sim():
    h('''create soma''')
    h.load_file("stdrun.hoc")
    h.soma.L = 5.6419
    h.soma.diam = 5.6419
    h.soma.insert("hh")
    ic = h.IClamp(h.soma(0.5))
    ic.delay = 0.5
    ic.dur = 0.1
    ic.amp = 0.3

    v = h.Vector()
    v.record(h.soma(0.5)._ref_v, sec=h.soma)
    tv = h.Vector()
    tv.record(h._ref_t, sec=h.soma)
    h.run()
    assert v[0] == -65.0


def test_spikes():
    ref_spikes = np.array([1.05])

    h('''create soma''')
    h.load_file("stdrun.hoc")
    h.soma.L = 5.6419
    h.soma.diam = 5.6419
    h.soma.insert("hh")
    ic = h.IClamp(h.soma(0.5))
    ic.delay = 0.5
    ic.dur = 0.1
    ic.amp = 0.3

    v = h.Vector()
    v.record(h.soma(0.5)._ref_v, sec=h.soma)
    tv = h.Vector()
    tv.record(h._ref_t, sec=h.soma)
    nc = h.NetCon(h.soma(0.5)._ref_v, None, sec=h.soma)
    spikestime = h.Vector()
    nc.record(spikestime)
    h.run()

    simulation_spikes = spikestime.to_python()

    assert np.allclose(simulation_spikes, ref_spikes)


def test_nrntest_test_2():
    h = hoc.HocObject()

    h(
        '''
    create axon, soma, dend[3]
    connect axon(0), soma(0)
    for i=0, 2 connect dend[i](0), soma(1)
    axon { nseg = 5  L=100 diam = 1 insert hh }
    forsec "dend" { nseg = 3 L=50 diam = 2 insert pas e_pas = -65 }
    soma { L=10 diam=10 insert hh }
    '''
    )

    assert h.axon.name() == "axon"
    assert str(h.dend) == "dend[?]"
    assert str(h.dend[1]) == "dend[1]"
    assert str(h.dend[1].name()) == "dend[1]"

    def e(stmt):
        try:
            exec(stmt)
        except Exception:
            assert (
                str(sys.exc_info()[0]) + ': ' + str(sys.exc_info()[1])
                == "<class 'TypeError'>: not assignable"
            )

    e('h.axon = 1')
    e('h.dend[1] = 1')

    assert str(h) == "<TopLevelHocInterpreter>"
    assert str(h.axon) == "axon"
    assert str(h.axon.name()) == "axon"
    assert str(h.axon(0.5)) == "axon(0.5)"
    assert str(h.axon(0.5).hh) == "hh"
    assert str(h.axon(0.5).hh.name()) == "hh"
    assert h.axon(0.5).hh.gnabar == 0.12

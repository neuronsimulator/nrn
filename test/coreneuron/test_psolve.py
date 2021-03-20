import pytest
import sys

from neuron import h, gui
pc = h.ParallelContext()

def model():
    pc.gid_clear()
    for s in h.allsec():
        h.delete_section(sec=s)
    s = h.Section()
    s.L = 10
    s.diam = 10
    s.insert("hh")
    ic = h.IClamp(s(.5))
    ic.delay = 0.1
    ic.dur = 0.1
    ic.amp = 0.5
    return (s, ic)

def test_psolve():
    # sequence of psolve with only beginning initialization
    m = model()
    vvec = h.Vector()
    h.tstop = 5
    vvec.record(m[0](.5)._ref_v, sec=m[0])

    h.run()
    vvec_std = vvec.c() # standard result

    def run(tstop):
      pc.set_maxstep(10)
      h.finitialize(-65)
      h.continuerun(1)
      while h.t < tstop:
        pc.psolve(h.t + 1)
    
    run(h.tstop)
    assert(vvec_std.eq(vvec))
    assert(vvec_std.size() == vvec.size())

    from neuron import coreneuron
    coreneuron.enable = True
    coreneuron.verbose = 0
    h.CVode().cache_efficient(True)
    run(h.tstop)
    assert(vvec_std.eq(vvec))
    assert(vvec_std.size() == vvec.size())
    coreneuron.enable = False

if __name__ == "__main__":
    test_psolve()

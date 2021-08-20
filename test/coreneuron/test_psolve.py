import os
import pytest
import sys
import traceback

enable_gpu = bool(os.environ.get("CORENRN_ENABLE_GPU", ""))

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
    ic = h.IClamp(s(0.5))
    ic.delay = 0.1
    ic.dur = 0.1
    ic.amp = 0.5 * 0
    syn = h.ExpSyn(s(0.5))
    nc = h.NetCon(None, syn)
    nc.weight[0] = 0.001
    return {"s": s, "ic": ic, "syn": syn, "nc": nc}


def test_psolve():
    # sequence of psolve with only beginning initialization
    m = model()
    vvec = h.Vector()
    h.tstop = 5
    vvec.record(m["s"](0.5)._ref_v, sec=m["s"])

    def run(tstop):
        pc.set_maxstep(10)
        h.finitialize(-65)
        m["nc"].event(3.5)
        m["nc"].event(2.6)
        h.continuerun(1)  # Classic NEURON so psolve starts at t>0
        while h.t < tstop:
            pc.psolve(h.t + 1)

    run(h.tstop)
    vvec_std = vvec.c()  # standard result

    from neuron import coreneuron

    coreneuron.enable = True
    coreneuron.verbose = 0
    coreneuron.gpu = enable_gpu
    h.CVode().cache_efficient(True)
    run(h.tstop)
    if vvec_std.eq(vvec) == 0:
        for i, x in enumerate(vvec_std):
            print("%.3f %g %g %g" % (i * h.dt, x, vvec[i], x - vvec[i]))
    assert vvec_std.eq(vvec)
    assert vvec_std.size() == vvec.size()
    coreneuron.enable = False


if __name__ == "__main__":
    try:
        test_psolve()
    except:
        traceback.print_exc()
        # Make the CTest test fail
        sys.exit(42)
    # The test doesn't exit without this.
    if enable_gpu:
        h.quit()

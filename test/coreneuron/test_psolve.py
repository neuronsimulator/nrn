from neuron.tests.utils.strtobool import strtobool
import os

import pytest

from neuron import h, gui


@pytest.fixture(scope="module", autouse=True)
def setup():
    global pc
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
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
    run(h.tstop)
    if vvec_std.eq(vvec) == 0:
        for i, x in enumerate(vvec_std):
            print(f"{i * h.dt:.3f} {x:g} {vvec[i]:g} {x - vvec[i]:g}")
    assert vvec_std.eq(vvec)
    assert vvec_std.size() == vvec.size()
    coreneuron.enable = False


def test_NetStim_noise():
    # Can use noiseFromRandom
    use_noiseFromRandom123 = False

    cells = {gid: (h.NetStim(), h.Random()) for gid in range(pc.id(), 5, pc.nhost())}
    for gid, cell in cells.items():
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(cell[0], None))
        cell[1].Random123(gid, 2, 3)
        cell[1].negexp(1)
        if use_noiseFromRandom123:
            cell[0].noiseFromRandom123(gid, 2, 3)
        else:
            cell[0].noiseFromRandom(cell[1])
        cell[0].interval = gid + 1
        cell[0].number = 100
        cell[0].start = 0
        cell[0].noise = 1

    spiketime = h.Vector()
    spikegid = h.Vector()
    pc.spike_record(-1, spiketime, spikegid)

    pc.set_maxstep(10)
    tstop = 100
    for cell in cells.values():
        cell[1].seq(0)  # only _ran_compat==2 initializes the streams
    h.finitialize()
    pc.psolve(tstop)
    spiketime_std = spiketime.c()
    spikegid_std = spikegid.c()
    print(f"spiketime_std.size {spiketime_std.size()}")

    spiketime.resize(0)
    spikegid.resize(0)
    from neuron import coreneuron

    coreneuron.verbose = 0
    coreneuron.enable = True
    for cell in cells.values():
        cell[1].seq(0)
    h.finitialize()
    while h.t < tstop:
        h.continuerun(h.t + 5)
        pc.psolve(h.t + 5)

    print(f"spiketime.size {spiketime.size()}")
    assert spiketime.eq(spiketime_std)
    assert spikegid.eq(spikegid_std)

    pc.gid_clear()


if __name__ == "__main__":
    test_psolve()
    test_NetStim_noise()
    for i in range(0):
        test_NetStim_noise()  # for checking memory leak
    h.quit()

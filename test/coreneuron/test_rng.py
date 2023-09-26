from neuron.tests.utils.strtobool import strtobool
import os

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

def test_NetStim_noise():
    # Can use noiseFromRandom

    cells = {gid: h.NetStim() for gid in range(pc.id(), 5, pc.nhost())}
    for gid, cell in cells.items():
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(cell, None))

        cell.noiseFromRandom123(gid, 2, 3)

        #cell.r1.init_rng(1,2,3)
        #cell.r2.init_rng(1,2,3)
        #cell.r1.set_seq(1,0)
        #cell.r2.set_seq(1,0)

        #cell.init_rng()

        #cell.init_rng(cell.r1, 1, 2)
        #cell.init_rng(cell.r2, 1, 2)
        #cell.init_rng(cell.r3, 1, 2)
        #cell.set_seq(cell.r, 10, 1)

        cell.interval = gid + 1
        cell.number = 100
        cell.start = 0
        cell.noise = 1

        cell.init_rng("r", 11, 12, 13);


    spiketime = h.Vector()
    spikegid = h.Vector()
    pc.spike_record(-1, spiketime, spikegid)

    pc.set_maxstep(10)
    tstop = 100

    #for cell in cells.values():
    #    cell.seq(0)  # only _ran_compat==2 initializes the streams

    h.finitialize()
    pc.psolve(tstop)
    spiketime_std = spiketime.c()
    spikegid_std = spikegid.c()
    print("spiketime_std.size %d" % spiketime_std.size())

    spiketime.resize(0)
    spikegid.resize(0)
    from neuron import coreneuron

    coreneuron.verbose = 0
    coreneuron.enable = False

    #for cell in cells.values():
    #    cell.set_seq(0)

    h.finitialize()
    while h.t < tstop:
        h.continuerun(h.t + 5)
        pc.psolve(h.t + 5)

    print("spiketime.size %d" % spiketime.size())
    assert spiketime.eq(spiketime_std)
    assert spikegid.eq(spikegid_std)

    pc.gid_clear()


if __name__ == "__main__":
    test_NetStim_noise()
    for i in range(0):
        test_NetStim_noise()  # for checking memory leak
    h.quit()

from math import isclose

from neuron import h
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
    cells = {gid: (h.NetStim()) for gid in range(pc.id(), 5, pc.nhost())}

    for gid, cell in cells.items():
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(cell, None))
        cell.noiseFromRandom123(gid, 2, 3)
        cell.interval = gid + 1
        cell.number = 100
        cell.start = 0
        cell.noise = 1

    spiketime = h.Vector()
    spikegid = h.Vector()
    pc.spike_record(-1, spiketime, spikegid)

    pc.set_maxstep(10)
    tstop = 5

    h.finitialize()
    pc.psolve(tstop)

    spiketime_result = spiketime.c()
    spikegid_result = spikegid.c()

    spikegid_ref = [1.0 ,0.0 ,2.0 ,0.0 ,0.0 ,0.0 ,4.0 ,3.0 ,0.0 ,4.0 ,1.0 ,0.0 ,0.0 ,2.0 ,2.0]
    spiketime_ref = [0.038647213491710304 ,0.08268113588796304 ,0.5931985927363619 ,0.7687313066056471 ,0.867367543646173 ,1.1822988033563793 ,1.3476448598895432 ,1.748395215773899 ,1.9382702939333631 ,2.3381219031177376 ,2.9858151911753863 ,3.3721447007688603 ,3.4714402733585277 ,4.130940076465841 ,4.406639959683753]

    for i in range(int(spiketime_result.size())):
        assert spikegid_ref[i] == spikegid_result[i] and isclose(spiketime_ref[i], spiketime_result[i])


def test_random():
    ncell = 10
    cells = []
    gids = range(pc.id(), ncell, pc.nhost())
    for gid in gids:
        pc.set_gid2node(gid, pc.id())
        cell = h.NetStim()

        cell.init_rng("rng_use", gid, 1, 2)
        cell.init_rng("rng_donotuse", gid, 2, 3)

        r1 = cell.sample_rng("rng_use")
        r2 = cell.sample_rng("rng_donotuse")

        assert r1 != 0 and r2 != 0 and r1 != r2

        print(" sample rng_use:: ", r1)
        print(" sample rng_donotuse:: ", r2)

        cells.append(cell)


if __name__ == "__main__":
    test_NetStim_noise()
    test_random()

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


def test_netstim_noise():
    cells = {gid: (h.NetStim()) for gid in range(pc.id(), 5, pc.nhost())}
    for gid, cell in cells.items():
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(cell, None))

        cell.interval = gid + 1
        cell.number = 100
        cell.start = 0
        cell.noise = 1

        # Initialize RANDOM variable ids and initial sequence.
        cell.ranvar.set_ids(gid, 2, 3).set_seq(0)

    spiketime = h.Vector()
    spikegid = h.Vector()
    pc.spike_record(-1, spiketime, spikegid)

    pc.set_maxstep(10)
    tstop = 5

    h.finitialize()
    pc.psolve(tstop)

    spiketime_result = spiketime.c()
    spikegid_result = spikegid.c()

    spikegid_ref = [
        1.0,
        0.0,
        2.0,
        0.0,
        0.0,
        0.0,
        4.0,
        3.0,
        0.0,
        4.0,
        1.0,
        0.0,
        0.0,
        2.0,
        2.0,
    ]
    spiketime_ref = [
        0.038647213491710304,
        0.08268113588796304,
        0.5931985927363619,
        0.7687313066056471,
        0.867367543646173,
        1.1822988033563793,
        1.3476448598895432,
        1.748395215773899,
        1.9382702939333631,
        2.3381219031177376,
        2.9858151911753863,
        3.3721447007688603,
        3.4714402733585277,
        4.130940076465841,
        4.406639959683753,
    ]

    # check if gid and spike time matches
    for i in range(int(spiketime_result.size())):
        assert spikegid_ref[i] == spikegid_result[i] and isclose(
            spiketime_ref[i], spiketime_result[i]
        )


def test_random():
    pc.gid_clear()
    ncell = 10
    cells = []
    gids = range(pc.id(), ncell, pc.nhost())
    rng = []
    for gid in gids:
        pc.set_gid2node(gid, pc.id())
        cell = h.NetStim()

        cell.ranvar.set_ids(gid, 2, 3).set_seq(0)

        r = cell.erand()
        rng.append(r)

    rng_ref = [
        0.08268113588796304,
        0.019323606745855152,
        0.19773286424545394,
        0.4370988039434747,
        0.26952897197790865,
        0.27785183823847076,
        1.4834024918566038,
        1.1439786359830195,
        0.13094521398166833,
        0.3743746759204156,
    ]

    for x, y in zip(rng, rng_ref):
        assert isclose(x, y)


if __name__ == "__main__":
    test_netstim_noise()
    test_random()

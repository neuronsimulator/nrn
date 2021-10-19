# arrange for InputPreSyn to be on queue after finitialize

from neuron import h

pc = h.ParallelContext()


def test_inputpresyn():
    # NetStim with gid = 1 connected to IntFire1 with gid = 2
    # sadly IntFire1 does not exist in coreneuron so use IntervalFire
    # make cells
    ncell = 10
    cells = {gid: None for gid in range(pc.id(), ncell, pc.nhost())}
    for gid in cells:
        pc.set_gid2node(gid, pc.id())
        cells[gid] = h.NetStim() if gid == 0 else h.IntervalFire()
        pc.cell(gid, h.NetCon(cells[gid], None))

    # connect
    netcons = {}
    for gid in cells:
        if gid != 0:
            netcons[(0, gid)] = nc = pc.gid_connect(0, cells[gid])
            nc.delay = 1
            nc.weight[0] = 2
        else:  # The NetStim
            ns = cells[gid]
            ns.start = 0
            ns.number = 1
            ns.interval = 10

    # does it look like what we want?
    for gid in cells:
        print(pc.id(), gid, cells[gid])
    for con in netcons:
        print(pc.id(), con)

    spiketime = h.Vector()
    gidvec = h.Vector()
    pc.spike_record(-1, spiketime, gidvec)

    def run(tstop):
        spiketime.resize(0)
        gidvec.resize(0)
        pc.set_maxstep(10)
        h.finitialize()
        h.fadvance()  # temporarily work around counting spike at 0 twice.
        pc.psolve(tstop)

    run(2)
    spiketime_std = spiketime.c()
    gidvec_std = gidvec.c()

    def same():
        assert spiketime_std.eq(spiketime)
        assert gidvec_std.eq(gidvec)

    h.CVode().cache_efficient(1)
    from neuron import coreneuron

    coreneuron.enable = 0
    coreneuron.verbose = 0
    run(2)
    same()

    pc.barrier()


if __name__ == "__main__":
    test_inputpresyn()

    if pc.nhost() > 1:
        pc.barrier()
        h.quit()

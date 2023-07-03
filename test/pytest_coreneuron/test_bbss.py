# More testing  BBSaveState.

from neuron import h

pc = h.ParallelContext()
cvode = h.CVode()


class Model1:
    def __init__(self, ncell, n):
        # gid -> [n NetCons] -> gid+1
        cells = {gid: h.Follower() for gid in range(pc.id(), ncell, pc.nhost())}
        for gid in cells:
            pc.set_gid2node(gid, pc.id())
            pc.cell(gid, h.NetCon(cells[gid], None))
        netcons = {}
        for gid in cells:
            netcons[gid] = [
                pc.gid_connect(gid - 1, cells[gid]) for _ in range(n) if gid > 0
            ]
        # Stimulate gid 0
        if 0 in cells:
            # stim cell[0] with fast burst of 5 spikes
            ns = h.NetStim()
            ns.start = 0.9999
            ns.number = 5
            ns.interval = 0.1
            nsnc = h.NetCon(ns, cells[0])
            nsnc.delay = 0
            nsnc.weight[0] = 0
        for gid, cell in cells.items():
            # cells generate spike on every input of weight > 1
            # cell.refrac = 0
            for i, nc in enumerate(netcons[gid]):
                nc.weight[0] = 0.001 * i + h.dt
                nc.delay = (1.0 + 0.01 * i) * h.dt

        self.cells = cells
        self.netcons = netcons
        self.ns = ns
        self.nsnc = nsnc


class Model2:
    """NetStim -> Cell -> N Cells -> Cell"""

    # exercise PreSyn on queue with use_min_delay_ by means of
    # several NetCon with same PreSyn and same delay
    def __init__(self, n):
        self.ncell_per_layer = [1, n, 2]
        self.cells = {}
        # make cells and associate with gid (round robin distribution)
        for ilayer, ncell in enumerate(self.ncell_per_layer):
            for icell in range(ncell):
                gid = self.info2gid(ilayer, icell)
                if (gid % pc.nhost()) == pc.id():
                    self.cells[gid] = h.Follower()
                    pc.set_gid2node(gid, pc.id())
                    pc.cell(gid, h.NetCon(self.cells[gid], None))

        # make connections (all to all from layer i-1 to layer i)
        self.netcons = {}
        for gid, cell in self.cells.items():
            ilayer, icell = self.gid2info(gid)
            if ilayer == 0:
                continue
            srclayer = ilayer - 1
            for isrc in range(self.ncell_per_layer[srclayer]):
                srcgid = self.info2gid(srclayer, isrc)
                nc = pc.gid_connect(srcgid, cell)
                nc.weight[0] = 0.1
                nc.delay = 1.0
                self.netcons[(srcgid, gid)] = nc

        # Stimulate layer 0 with NetStim
        self.netstims = {}
        self.netstim_con = {}
        for gid in range(self.ncell_per_layer[0]):
            if gid in self.cells:
                ns = h.NetStim()
                self.netstims[gid] = ns
                ns.start = 0.9999
                ns.number = 5
                ns.interval = 0.1
                nc = h.NetCon(ns, self.cells[gid])
                nc.delay = 0.0
                nc.weight[0] = 0.1
                self.netstim_con[gid] = nc
        self.ns = self.netstims[0]
        self.nsnc = self.netstim_con[0]

        print(self.cells)
        print(self.netcons)
        print(self.netstim_con)

    def info2gid(self, ilayer, icell):
        return ilayer * 100 + icell

    def gid2info(self, gid):
        return (int(gid / 100)), gid % 100


def test_bbss():
    print("focus on BinQ initialization Issue #1444")
    ncell = 3
    n = 5

    model = Model2(5)

    #    pc.gid_clear()
    #    model = Model1(ncell, n)

    spiketime = h.Vector()
    spikegid = h.Vector()
    pc.spike_record(-1, spiketime, spikegid)

    def run(tstop):
        pc.set_maxstep(10)
        h.finitialize()
        pc.psolve(tstop)

    spiketime_std = spiketime.c()
    spikegid_std = spikegid.c()

    def set_stdspikes():
        spiketime_std.copy(spiketime)
        spikegid_std.copy(spikegid)

    tstop = 5
    run(tstop)
    set_stdspikes()

    def prspikes():
        print("prspikes")
        for i, gid in enumerate(spikegid):
            print("%g %d" % (spiketime[i], gid))

    def compare_spikes():
        x = list(zip(spiketime_std, spikegid_std))
        x = sorted(x, key=lambda e: e[1])
        y = list(zip(spiketime, spikegid))
        y = sorted(y, key=lambda e: e[1])
        if len(x) != len(y):
            print(len(x), len(y))
        # assert len(x) == len(y)
        if x != y:
            q = (x, y) if len(x) <= len(y) else (y, x)
            for i, a in enumerate(q[0]):
                b = q[1][i]
                if a != b:
                    # assert a[1] == b[1]
                    z = abs(a[0] - b[0])
                    if z >= 1e-9:
                        print(x[i], y[i])
                    assert z < 1e-9

    def srun(tsave, tstop):
        qm = cvode.queue_mode()
        print(
            "srun tsave=%g tstop=%g start=%g delay=%g %s"
            % (tsave, tstop, model.ns.start, model.nsnc.delay, "binq" if qm else "")
        )
        run(tsave)
        st = spiketime.c()
        sg = spikegid.c()
        bbss = h.BBSaveState()
        bbss.save("allcell_bbss.dat")
        qm = 1
        if qm:
            print("after save")
            cvode.print_event_queue()
        h.finitialize(-65)
        spiketime.resize(0)
        spikegid.resize(0)
        bbss.restore("allcell_bbss.dat")
        if qm:
            print("after restore")
            cvode.print_event_queue()
        pc.psolve(tstop)
        st.append(spiketime)
        sg.append(spikegid)
        spiketime.copy(st)
        spikegid.copy(sg)
        print("    %d spikes" % len(spiketime))

    srun(1.1, tstop)
    compare_spikes()

    cvode.queue_mode(1)
    run(tstop)
    assert len(spiketime_std) == len(spiketime)
    spiketime_std = spiketime.c()
    spikegid_std = spikegid.c()

    srun(1.1, tstop)
    compare_spikes()
    for binq in [0, 1]:
        cvode.queue_mode(binq)
        parms = [
            (1, 0),
            (1e-10, 0),
            (0, 0),
            (0.25 * h.dt, 0),
            (0.5 * h.dt, 0),
            (0.75 * h.dt, 0),
        ]
        for parm in parms:
            if 0 in model.cells:
                model.ns.start, model.nsnc.delay = parm
            for tsave in [0.0, h.dt]:
                run(tstop)
                set_stdspikes()
                srun(tsave, tstop)
                compare_spikes()

    cvode.queue_mode(0)
    pc.gid_clear()


if __name__ == "__main__":
    test_bbss()

# Test of data return covering most of the functionality.
import os
import pytest
import sys
import traceback

enable_gpu = bool(os.environ.get("CORENRN_ENABLE_GPU", ""))

from neuron import h

pc = h.ParallelContext()
h.dt = 1.0 / 32
cvode = h.CVode()


class Cell:
    def __init__(self, gid):
        nsec = 5
        self.gid = gid
        r = h.Random()
        self.r = r
        r.Random123(gid, 0, 0)
        self.secs = [h.Section(name="s%d" % i, cell=self) for i in range(nsec)]
        s0 = self.secs[0]

        # random connect to exercise permute
        for i, s in enumerate(self.secs[1:]):
            s.connect(self.secs[int(r.discunif(0, i))])

        # hh and pas everywhere with random gkbar_hh and g_pas for intrinsic firing.
        # everywhere but s0.
        s = self.secs[0]
        s.L = 10
        s.diam = 10
        s.insert("hh")
        for s in self.secs[1:]:
            s.nseg = 4
            s.L = 50
            s.diam = 1
            s.insert("pas")
            s.e_pas = -65
            s.g_pas = r.uniform(0.0001, 0.0002)
            s.insert("hh")
            s.gkbar_hh = r.uniform(0.01, 0.02)

        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(s0(0.5)._ref_v, None, sec=s0))

        # add a few random Netstim -> NetCon -> Exp2Syn connections to verify
        # correct data return for ARTIFICIAL_CELL and POINT_PROCESS
        nsyn = 3
        self.stims = [h.NetStim() for _ in range(nsyn)]
        self.syns = [
            h.Exp2Syn(self.secs[int(r.discunif(0, nsec - 1))](0.5)) for _ in range(nsyn)
        ]
        self.ncs = [h.NetCon(self.stims[i], self.syns[i]) for i in range(nsyn)]
        for stim in self.stims:
            stim.start = int(r.uniform(0, 1) / h.dt) * h.dt
            stim.interval = int(r.uniform(1, 2) / h.dt) * h.dt
        for nc in self.ncs:
            nc.weight[0] = r.uniform(0.0, 0.001)
            nc.delay = int(r.discunif(5, 20)) * h.dt

    def __str__(self):
        return "Cell%d" % self.gid


class Network:
    def __init__(self):
        self.cells = [Cell(i) for i in range(5)]
        cvode.use_fast_imem(True)
        # a few intrinsically firing ARTIFICIAL_CELLS with and without gids
        self.acells = [h.IntervalFire() for _ in range(8)]
        r = h.Random()
        r.Random123(6, 0, 0)
        for a in self.acells:
            a.tau = r.uniform(2, 5)
            a.invl = r.uniform(2, 4)
        for i, a in enumerate(self.acells[5:]):
            pc.set_gid2node(i + 5, pc.id())
            pc.cell(i + 5, h.NetCon(a, None))

    def add_data(self, mname, d):
        instances = h.List(mname)
        if instances.count() == 0:
            return
        names = []
        inst = instances.o(0)
        for name in dir(inst):
            if "__" not in name:
                try:
                    if type(getattr(inst, name)) == float:
                        names.append(name)
                except:
                    pass
        for inst in instances:
            for name in names:
                d.append(getattr(inst, name))

    def data(self):
        d = [h.t]
        for sec in h.allsec():
            for seg in sec:
                d.append(seg.v)
                d.append(seg.i_membrane_)
                for mech in seg:
                    for var in mech:
                        d.append(var[0])

        # all NetStim, ExpSyn, ...
        for mname in ["NetStim", "Exp2Syn", "IntervalFire"]:
            self.add_data(mname, d)

        return d


show = False


def mkgraphs(model):
    h.newPlotV()
    gcell = h.graphList[0].o(h.graphList[0].count() - 1)
    gcell.erase_all()
    for cell in model.cells:
        for sec in cell.secs:
            gcell.addvar(" ", sec(0.5)._ref_v)

    h.newPlotV()
    gacell = h.graphList[0].o(h.graphList[0].count() - 1)
    gacell.erase_all()
    gacell.size(0, h.tstop, 0, h.tstop)
    for acell in model.acells:
        gacell.addvar(" ", acell._ref_t0)
    model.gui = (gcell, gacell)


def test_datareturn():
    from neuron import coreneuron

    coreneuron.enable = False

    model = Network()

    tstop = 5

    pc.set_maxstep(10)

    def run(tstop):
        h.finitialize(-65)
        pc.psolve(tstop)

    if show:
        # visual verify that cells and acells have different final results so that
        # data return comparisons are substantive.
        from neuron import gui

        h.steps_per_ms = 1.0 / h.dt
        h.setdt()
        h.tstop = tstop
        mkgraphs(model)
        h.run()
    else:
        run(tstop)  # NEURON run
    std = model.data()

    print("CoreNEURON run")
    h.CVode().cache_efficient(1)
    coreneuron.enable = True
    coreneuron.gpu = enable_gpu

    coreneuron.cell_permute = 0
    run(tstop)
    tst = model.data()
    max_unpermuted = h.Vector(std).sub(h.Vector(tst)).abs().max()
    print("max diff unpermuted = %g" % max_unpermuted)

    coreneuron.cell_permute = 1
    run(tstop)
    tst = model.data()
    max_permuted = h.Vector(std).sub(h.Vector(tst)).abs().max()
    print("max diff permuted = %g" % max_permuted)

    pc.nthread(2)
    run(tstop)
    tst = model.data()
    max_permuted_thread = h.Vector(std).sub(h.Vector(tst)).abs().max()

    coreneuron.enable = False

    print(
        "max diff permuted with %d threads = %g" % (pc.nthread(), max_permuted_thread)
    )

    assert max_unpermuted < 1e-10
    assert max_permuted < 1e-10
    assert max_permuted_thread < 1e-10

    if __name__ != "__main__":
        # tear down
        pc.nthread(1)
        pc.gid_clear()
        return None

    return model


if __name__ == "__main__":
    show = False
    try:
        model = test_datareturn()
    except:
        traceback.print_exc()
        # Make the CTest test fail
        sys.exit(42)
    # The test doesn't exit without this.
    if enable_gpu:
        h.quit()

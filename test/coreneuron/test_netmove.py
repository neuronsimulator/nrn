# Basically want to test that net_move statement doesn't get
# mixed up with other instances.

import os
import pytest
import traceback

enable_gpu = bool(os.environ.get("CORENRN_ENABLE_GPU", ""))

from neuron import h

h.load_file("stdrun.hoc")

pc = h.ParallelContext()
h.dt = 1.0 / 4
h.steps_per_ms = 4


class Cell:
    def __init__(self, gid):
        self.soma = h.Section(name="soma", cell=self)
        self.gid = gid
        pc.set_gid2node(gid, pc.id())
        self.r = h.Random()
        self.r.Random123(gid, 0, 0)
        self.syn = h.DAsyn(self.soma(0.5))
        pc.cell(gid, h.NetCon(self.soma(0.5)._ref_v, None, sec=self.soma))
        # random start times for the internal events
        self.syn.tau0 = 10 * self.r.uniform(0.9, 1.1)
        self.syn.tau1 = 3 * self.r.uniform(0.9, 1.1)
        self.syn.dur = 6 * self.r.uniform(1, 10)

        self.ns = h.NetStim()
        self.ns.noise = 1
        self.ns.start = 0
        self.ns.number = 1e9
        self.ns.interval = 6
        self.ns.noiseFromRandom123(gid, 0, 1)
        self.nc = h.NetCon(self.ns, self.syn)
        self.nc.weight[0] = 0.1 * self.r.uniform(0.9, 1.1)
        self.nc.delay = 1
        self.msgvec = h.Vector()
        self.msgvec.record(self.syn._ref_msg, sec=self.soma)

    def result(self):
        return (self.syn.n_netsend, self.syn.n_netmove, self.msgvec.c())


def test_netmove():
    from neuron import coreneuron

    coreneuron.enable = False

    ncell = 10
    gids = range(pc.id(), ncell, pc.nhost())  # round robin

    cells = [Cell(gid) for gid in gids]

    tstop = 100

    def run(tstop, mode):
        pc.set_maxstep(10)
        h.finitialize(-65)
        if mode == 0:
            pc.psolve(tstop)
        elif mode == 1:
            while h.t < tstop:
                pc.psolve(h.t + 1.0)
        else:
            while h.t < tstop:
                h.continuerun(h.t + 0.5)
                pc.psolve(h.t + 0.5)

    run(tstop, 0)  # NEURON run

    stdlist = [cell.result() for cell in cells]

    print("CoreNEURON run")
    h.CVode().cache_efficient(1)
    coreneuron.enable = True
    coreneuron.verbose = 0
    coreneuron.gpu = enable_gpu

    def runassert(mode):
        run(tstop, mode)
        for i, cell in enumerate(cells):
            result = cell.result()
            std = stdlist[i]
            for j in range(2):
                assert std[j] == result[j]
            assert std[2].eq(result[2])

    for mode in [0, 1, 2]:
        runassert(mode)

    coreneuron.enable = False
    # teardown
    pc.gid_clear()
    return stdlist


if __name__ == "__main__":
    try:
        from neuron import gui

        stdlist = test_netmove()
        g = h.Graph()
        print("n_netsend  n_netmove")
        for result in stdlist:
            print(result[0], result[1])
            result[2].line(g)
        g.exec_menu("View = plot")
    except:
        traceback.print_exc()
        # Make the CTest test fail
        sys.exit(42)
    # The test doesn't exit without this.
    if enable_gpu:
        h.quit()

# Basically want to test that net_move statement doesn't get
# mixed up with other instances.
from neuron.tests.utils.strtobool import strtobool
import os

from neuron import h

h.load_file("stdrun.hoc")

pc = h.ParallelContext()
h.dt = 1.0 / 4
h.steps_per_ms = 4


class Cell:
    def __init__(self, gid):
        self.soma = h.Section(name="soma", cell=self)
        if gid % 2 == 0:
            # CoreNEURON permutation not the identity if cell topology not homogeneous
            self.dend = h.Section(name="dend", cell=self)
            self.dend.connect(self.soma(0.5))
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
    coreneuron.enable = True
    coreneuron.verbose = 0
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))

    def runassert(mode):
        run(tstop, mode)
        for i, cell in enumerate(cells):
            result = cell.result()
            std = stdlist[i]
            for j in range(2):
                if std[j] != result[j]:
                    print(
                        "cell {} std[{}]={} result[{}]={}".format(
                            i, j, std[j], j, result[j]
                        )
                    )
                    assert False
            if not std[2].eq(result[2]):
                print("Cell", i, "mode", mode)
                print(std[2], len(std[2]), result[2], len(result[2]))
                print("NEURON")
                print(list(std[2]))
                print("CoreNEURON")
                print(list(result[2]))
                assert False

    for mode in [0, 1, 2]:
        runassert(mode)

    coreneuron.enable = False
    # teardown
    pc.gid_clear()
    return stdlist


if __name__ == "__main__":
    from neuron import gui

    stdlist = test_netmove()
    g = h.Graph()
    print("n_netsend  n_netmove")
    for result in stdlist:
        print(result[0], result[1])
        result[2].line(g)
    g.exec_menu("View = plot")
    h.quit()

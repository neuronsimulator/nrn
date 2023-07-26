# Basically want to test that FOR_NETCONS statement works when
# the NetCons connecting to ForNetConTest instances are created
# in random order.
from neuron.tests.utils.strtobool import strtobool
import os

from neuron import h

h.load_file("stdrun.hoc")

pc = h.ParallelContext()
h.dt = 1.0 / 32


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
        self.syn = h.ForNetConTest(self.soma(0.5))
        pc.cell(gid, h.NetCon(self.syn, None))
        # random start times for the internal events
        self.syn.tbegin = self.r.discunif(0, 100) * h.dt


def test_fornetcon():
    from neuron import coreneuron

    coreneuron.enable = False

    ncell = 10
    gids = range(pc.id(), ncell, pc.nhost())  # round robin

    cells = [Cell(gid) for gid in gids]
    nclist = []

    # src first to more easily randomize NetCon creation order
    # so that NetCon to target not all adjacent
    for srcgid in range(ncell):
        for tarcell in cells:
            if int(tarcell.r.discunif(0, 1)) == 1:
                nclist.append(pc.gid_connect(srcgid, tarcell.syn))
                nclist[-1].delay = tarcell.r.discunif(10, 50) * h.dt
                nclist[-1].weight[0] = srcgid * 10000 + tarcell.gid

    spiketime = h.Vector()
    spikegid = h.Vector()
    pc.spike_record(-1, spiketime, spikegid)

    tstop = 8

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

    spiketime_std = spiketime.c()
    spikegid_std = spikegid.c()

    def get_weights():
        weight = []
        for nc in nclist:
            w = nc.weight
            weight.append((w[0], w[1], w[2], w[3], w[4]))
        return weight

    weight_std = get_weights()

    print("CoreNEURON run")
    coreneuron.enable = True
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))

    def runassert(mode):
        spiketime.resize(0)
        spikegid.resize(0)

        run(tstop, mode)
        assert len(spiketime) > 0
        assert spiketime_std.eq(spiketime) == 1.0
        assert spikegid_std.eq(spikegid) == 1.0
        assert len(weight_std) > 0
        assert weight_std == get_weights()

    for mode in [0, 1, 2]:
        runassert(mode)

    coreneuron.enable = False

    # help cover/test_netcvode.cpp cover the NetCon.setpost method
    # with respect to a change in weight vector size
    assert len(nclist[0].weight) == 5
    a = h.IntFire1()
    nclist[0].setpost(a)
    assert len(nclist[0].weight) == 1

    # teardown
    pc.gid_clear()


if __name__ == "__main__":
    test_fornetcon()
    h.quit()

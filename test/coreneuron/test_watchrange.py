# Basically want to test that net_move statement doesn't get
# mixed up with other instances.
# Augmented to also test RANDOM (Bounce2) with coreneuron permutation.
from neuron.tests.utils.strtobool import strtobool
import os

from neuron import h

h.load_file("stdrun.hoc")

pc = h.ParallelContext()
h.steps_per_ms = 8
h.dt = 1.0 / h.steps_per_ms


class Cell:
    def __init__(self, gid, bounce=0):
        self.soma = h.Section(name="soma", cell=self)
        if gid % 2 == 0:
            # CoreNEURON permutation not the identity if cell topology not homogeneous
            self.dend = h.Section(name="dend", cell=self)
            self.dend.connect(self.soma(0.5))
        self.gid = gid
        pc.set_gid2node(gid, pc.id())
        if bounce == 0:
            self.syn = h.Bounce(self.soma(0.5))
            self.syn.noiseFromRandom123(gid, 0, 1)
        else:
            self.syn = h.Bounce2(self.soma(0.5))
            self.syn.ran.set_ids(gid, 0, 1)
        pc.cell(gid, h.NetCon(self.soma(0.5)._ref_v, None, sec=self.soma))
        self.t1vec = h.Vector()
        self.t1vec.record(self.syn._ref_t1, sec=self.soma)
        self.xvec = h.Vector()
        self.xvec.record(self.syn._ref_x, sec=self.soma)
        self.rvec = h.Vector()
        self.rvec.record(self.syn._ref_r, sec=self.soma)

    def result(self):
        return (
            self.syn.n_high,
            self.syn.n_mid,
            self.syn.n_low,
            self.t1vec.c(),
            self.xvec.c(),
            self.rvec.c(),
        )


def watchrange():
    from neuron import coreneuron

    coreneuron.enable = False

    ncell = 10
    gids = range(pc.id(), ncell, pc.nhost())  # round robin

    cells = [Cell(gid) for gid in gids]

    # complete the coverage of netcvode.cpp static void steer_val
    # Just so happens that Bounce declares an x var that does not get
    # mirrored by NetCon.x
    nc = h.NetCon(cells[2].syn, None)
    cells[2].syn.x = 0.1
    nc.x = 2.0
    assert nc.x == 0.0
    assert cells[2].syn.x == 0.1
    del nc

    # @olupton changed from 20 to trigger assert(datum==2) failure.
    tstop = 1.0

    def run(tstop, mode):
        pc.set_maxstep(10)
        h.finitialize(-65)
        if mode == 0:
            pc.psolve(tstop)
        elif mode == 1:
            while h.t < tstop:
                pc.psolve(h.t + h.dt)
        else:
            while h.t < tstop:
                h.continuerun(h.t + h.dt)
                pc.psolve(h.t + h.dt)

    tvec = h.Vector()
    tvec.record(h._ref_t, sec=cells[0].soma)
    run(tstop, 0)  # NEURON run
    tvec = tvec.c()  # don't record again but save.

    stdlist = [cell.result() for cell in cells]

    print("CoreNEURON run")
    coreneuron.enable = True
    coreneuron.verbose = 0
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))

    def runassert(mode):
        run(tstop, mode)
        hml = ["invalid", "low", " mid", " high"]
        for i, cell in enumerate(cells):
            result = cell.result()
            std = stdlist[i]
            # Organised this way so we get a better overview of what went wrong
            # when something fails.
            success = all(std[j] == result[j] for j in range(3)) and all(
                std[j].eq(result[j]) for j in range(3, 6)
            )
            if not success:
                print("mode=" + str(mode))
                for j in range(3):
                    if std[j] != result[j]:
                        print(
                            "cell=%d %s:(nrn: %d cnrn: %d)"
                            % (i, ("high", "mid", "low")[j], std[j], result[j])
                        )
                # Look at the first place the flag value differs
                k = int(std[4].c().sub(result[4]).indwhere("!=", 0))
                print(
                    "first difference at %d (%g, %s, r=%g) vs (%g, %s, r=%g)"
                    % (
                        k,
                        std[3][k],
                        hml[int(std[4][k])],
                        std[5][k],
                        result[3][k],
                        hml[int(result[4][k])],
                        result[5][k],
                    )
                )
                for ik in range(k + 1):
                    print(
                        "  %d %g nrn(%g, %s, r=%g) vs cnrn(%g, %s, r=%g)"
                        % (
                            ik,
                            tvec[ik],
                            std[3][ik],
                            hml[int(std[4][ik])],
                            std[5][ik],
                            result[3][ik],
                            hml[int(result[4][ik])],
                            result[5][ik],
                        )
                    )

            assert success

    for mode in [0, 1, 2]:
        runassert(mode)

    # replace Bounce with Bounce2 (Uses RANDOM declaration)
    pc.gid_clear()
    cells = [Cell(gid, bounce=2) for gid in gids]
    for mode in [0, 1, 2]:
        runassert(mode)

    coreneuron.enable = False
    # teardown
    pc.gid_clear()
    return stdlist, tvec


def test_watchrange():
    watchrange()


if __name__ == "__main__":
    from neuron import gui

    stdlist, tvec = watchrange()
    g = h.Graph()
    print("n_high  n_mid  n_low")
    for i, result in enumerate(stdlist):
        print(result[0], result[1], result[2])
        result[4].line(g, tvec, i, 2)
    g.exec_menu("View = plot")
    h.quit()

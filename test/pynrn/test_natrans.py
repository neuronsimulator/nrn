from neuron import coreneuron
from neuron.tests.utils import (
    cache_efficient,
    num_threads,
    parallel_context,
)
import pytest


class Cell:
    """Cell class used for test_natrans."""

    def __init__(self, id):
        from neuron import h

        self.id = id
        self.soma = h.Section(name="soma", cell=self)
        self.soma.diam = 5
        self.soma.L = 5
        self.soma.insert("na_ion")
        # finitialize leaves nai as is. Otherwise cannot pass interpreter setting onto coreneuron unless use a mod file that WRITE nai in INITIAL via parameter
        h.ion_style("na_ion", 1, 0, 0, 0, 0)

    def __str__(self):
        return "Cell[{:d}]".format(self.id)


configs = ["neuron"]
if coreneuron.available:
    configs.append("coreneuron")
    if coreneuron.gpu_available:
        configs.append("coreneuron-gpu")


@pytest.mark.parametrize("config", configs)
def test_natrans(config):
    """Want to exercise the internal indexing schemes. So need mpi and threads.

    * Random order of sgid calls to source_var.
    * Random order and location of sgid source.
    * Random order and location of sgid target.
    * An sgid source has random number of targets.

    Might be a good idea to add some voltage sources and ki sources.
    """
    from neuron import h

    with parallel_context() as pc, num_threads(pc, 4), cache_efficient(
        True
    ), coreneuron(enable="coreneuron" in config, gpu="gpu" in config):
        rank = pc.id()
        nhost = pc.nhost()

        ncell = 100
        nsrc = 50  # so that number of sgid pointing to _ref_nai
        # multiple sgid may NOT point to same _ref_nai
        ntarget = 200  # so sgid on average sends to ntarget/nsrc targets.

        cells = []
        for gid in range(rank, ncell, nhost):
            pc.set_gid2node(gid, rank)
            cells.append(Cell(gid))
            pc.cell(gid, h.NetCon(cells[-1].soma(0.5)._ref_v, None, sec=cells[-1].soma))

        r = h.Random()
        r.Random123(1, 1, 0)

        # nsrc unique random sgids in range(ncell). Sample without replacement.
        sgids = []
        v = list(range(ncell))
        for i in range(nsrc):
            x = int(r.discunif(0, len(v) - 1))
            sgids.append(v[x])
            v.pop(x)

        for sgid in sgids:
            if pc.gid_exists(sgid) == 3:
                sec = pc.gid2cell(sgid).soma
                pc.source_var(sec(0.5)._ref_nai, sgid, sec=sec)

        # ntarget randomly chosen cells are targets for the nsrc sgids
        # ntarget NaTrans are created
        targets = []
        for itar in range(ntarget):
            gid = int(r.discunif(0, ncell - 1))
            sgid = sgids[int(r.discunif(0, nsrc - 1))]
            if pc.gid_exists(gid) == 3:
                sec = pc.gid2cell(gid).soma
                target = h.NaTrans(sec(0.5))
                targets.append(target)
                pc.target_var(target, target._ref_napre, sgid)
                target.sgid = sgid

        pc.set_maxstep(10)
        pc.setup_transfer()

        h.dt = 0.1
        tstop = 0.1

        def run():
            h.finitialize(-65)
            for sgid in sgids:
                if pc.gid_exists(sgid) == 3:
                    sec = pc.gid2cell(sgid).soma
                    sec(0.5).nai = float(sgid) / 100.0 + 0.001
            tars = h.List("NaTrans")
            for tar in tars:
                tar.napre = 0.0001  # correct values don't carryover from previous sim
            pc.psolve(tstop)
            for tar in tars:
                x = (tar.sgid / 100.0 + 0.001) if tar.sgid >= 0.0 else 0.0
                diff = abs(tar.napre - x)
                threshold = 1e-10
                if diff > threshold:
                    print(
                        "%d %s %g %g %g" % (rank, tar.hname(), tar.sgid, tar.napre, x)
                    )
                assert diff <= threshold

        # NEURON: fails if tar.napre not what is expected
        # CoreNEURON: fails if does not copy expected tar.napre to NEURON
        run()


if __name__ == "__main__":
    # python test_natrans.py will run all the tests in this file
    # e.g. __file__ --> __file__ + "::test_foo" would just run test_foo
    pytest.main([__file__])

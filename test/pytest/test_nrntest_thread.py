"""
Tests that used to live in the thread/ subdirectory of the
https://github.com/neuronsimulator/nrntest repository
"""
import os
import pytest
from neuron import h
from neuron.tests.utils import (
    num_threads,
    parallel_context,
)
from neuron.tests.utils.checkresult import Chk


@pytest.fixture(scope="module")
def chk():
    """Manage access to JSON reference data."""
    dir_path = os.path.dirname(os.path.realpath(__file__))
    checker = Chk(os.path.join(dir_path, "test_nrntest_thread.json"))
    yield checker
    # Save results to disk if they've changed; this is called after all tests
    # using chk have executed
    checker.save()


class Cell:
    def __init__(self, id, ncell):
        self.id = id
        self.soma = h.Section(name="soma", cell=self)
        self.soma.pt3dclear()
        self.soma.pt3dadd(0, 0, 0, 1)
        self.soma.pt3dadd(15, 0, 0, 1)
        self.soma.L = self.soma.diam = 5.6419
        self.soma.insert("MCna")
        for seg in self.soma:
            seg.MCna.gnabar = 0.12
            seg.MCna.lp = 1.9
            seg.MCna.ml = 0.75
            seg.MCna.nm = 0.3
        self.sc = h.SEClamp(self.soma(0.5))
        self.sc.dur1 = 100
        self.sc.amp1 = (100 * id / ncell) - 50
        self.ina_vec = h.Vector()
        self.ina_vec.record(self.soma(0.5)._ref_ina)
        self.tv = h.Vector()
        self.tv.record(h._ref_t)

    def __str__(self):
        return "Cell[{:d}]".format(self.id)

    def data(self):
        return {"ina": list(self.ina_vec), "t": list(self.tv)}


# TODO: fix coreneuron compilation and execution of this test, see
#       https://github.com/neuronsimulator/nrn/issues/2397.
simulators = ["neuron"]


@pytest.mark.parametrize("simulator", simulators)
@pytest.mark.parametrize("threads", [1, 3])
def test_mcna(chk, simulator, threads):
    """
    Derived from nrntest/thread/mcna.hoc, which used to be run with neurondemo.
    Old comment "test GLOBAL counter".
    """
    tstop = 5  # ms
    ncell = 10
    cells = [Cell(id, ncell) for id in range(ncell)]
    with parallel_context() as pc, num_threads(pc, threads=threads):
        pc.set_maxstep(10)
        h.finitialize()
        # the nrn_cur kernel gets called once in finitialize, and it calls the
        # code that increments cnt1 twice
        assert h.cnt1_MCna == 2 * ncell
        assert h.cnt2_MCna == 3
        pc.psolve(tstop)
    time_steps = round(tstop / h.dt)
    assert h.cnt1_MCna == 2 * ncell * (time_steps + 1)  # +1 b/c of finitialize
    assert h.cnt2_MCna == 2 * ncell * time_steps + 3
    t_vector = None
    model_data = {}
    cell_names = set()
    for n, cell in enumerate(cells):
        cell_data = cell.data()
        # The time vector should be identical for all cells
        cell_times = cell_data.pop("t")
        assert t_vector is None or t_vector == cell_times
        t_vector = cell_times
        # The other data should vary across cells; just check first/last
        if n == 0 or n == ncell - 1:
            model_data[str(cell)] = cell_data
            cell_names.add(str(cell))
    model_data["t"] = t_vector
    # Make sure the whole model is deleted. If this test breaks in future with
    # mismatches in cnt1 between the first execution (e.g. 1 thread) and later
    # executions (e.g. 3 threads) then that might be because this has broken
    # and the later executions include relics of the earlier ones.
    del cell, cells
    ref_data = chk.get("mcna", None)
    if ref_data is None:  # pragma: no cover
        # bootstrapping
        chk("mcna", model_data)
        return
    # Compare this run to the reference data.
    assert model_data["t"] == ref_data["t"]
    for cell_name in cell_names:
        assert model_data[cell_name]["ina"] == pytest.approx(
            ref_data[cell_name]["ina"], abs=1e-15, rel=5e-10
        )


if __name__ == "__main__":
    # python test_nrntest_thread.py will run all the tests in this file
    # e.g. __file__ --> __file__ + "::test_mcna" would just run test_mcna
    pytest.main([__file__])

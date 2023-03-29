"""
Tests that used to live in the thread/ subdirectory of the
https://github.com/neuronsimulator/nrntest repository
"""
from itertools import product
import os
import pytest
from neuron import coreneuron, h
from neuron.config import arguments
from neuron.tests.utils import (
    cache_efficient,
    num_threads,
    parallel_context,
)
from neuron.tests.utils.checkresult import Chk


@pytest.fixture(scope="module")
def chk():
    """Manage access to JSON reference data."""
    dir_path = os.path.dirname(os.path.realpath(__file__))
    checker = Chk(os.path.join(dir_path, "test_nrntest_thread.json"), must_exist=False)
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


simulators = ["neuron"] + ["coreneuron"] if arguments["NRN_ENABLE_CORENEURON"] else []


@pytest.mark.parametrize("simulator", simulators)
@pytest.mark.parametrize("threads", [1, 3])
def test_mcna(chk, simulator, threads):
    """
    Derived from nrntest/thread/mcna.hoc, which used to be run with neurondemo.
    Old comment "test GLOBAL counter".
    """
    ncell = 10
    cells = [Cell(id, ncell) for id in range(ncell)]
    enable_coreneuron = simulator == "coreneuron"
    with cache_efficient(enable_coreneuron), coreneuron(
        enable=enable_coreneuron, verbose=0
    ), parallel_context() as pc, num_threads(pc, threads=threads):
        pc.set_maxstep(10)
        h.finitialize()
        pc.psolve(5)
    t_vector = None
    model_data = {}
    model_data["cnt1"] = h.cnt1_MCna
    model_data["cnt2"] = h.cnt2_MCna
    for cell in cells:
        cell_data = cell.data()
        # The time vector should be identical for all cells
        cell_times = cell_data.pop("t")
        assert t_vector is None or t_vector == cell_times
        t_vector = cell_times
        # The other data should vary across cells
        model_data[str(cell)] = cell_data
    model_data["t"] = t_vector
    del cells
    ref_data = chk.get("mcna", None)
    if ref_data is None:
        # bootstrapping
        chk("mcna", model_data)
        return
    if simulator == "coreneuron":
        # CoreNEURON does not handle GLOBAL variables that are updated during
        # simulation.
        ref_data = ref_data.copy()
        for key in ("cnt1", "cnt2"):
            ref_data.pop(key)
            model_data.pop(key)
    assert model_data == ref_data


if __name__ == "__main__":
    # python test_nrntest_fast.py will run all the tests in this file
    # e.g. __file__ --> __file__ + "::test_t13" would just run test_t13
    pytest.main([__file__])

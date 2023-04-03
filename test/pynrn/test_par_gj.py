from neuron import h
from neuron.tests.utils import (
    parallel_context,
    sparse_partrans,
    time_step,
)
from neuron.tests.utils.checkresult import Chk
import os
import pytest
import time


@pytest.fixture(scope="module")
def chk():
    """Manage access to JSON reference data."""
    dir_path = os.path.dirname(os.path.realpath(__file__))
    checker = Chk(os.path.join(dir_path, "test_par_gj.json"), must_exist=False)
    yield checker
    # Save results to disk if they've changed; this is called after all tests
    # using chk have executed
    checker.save()


class Cell:
    def __repr__(self):
        return "Cell[%d]" % self.id

    def __init__(self, id):
        self.id = id
        # create the morphology and connect it
        self.soma = h.Section(name="soma", cell=self)
        self.dend = h.Section(name="dend", cell=self)
        self.dend.connect(self.soma(0.5))
        self.soma.insert("pas")
        self.dend.insert("pas")
        self.dend(0.5).pas.e = -65
        self.soma(0.5).pas.e = -65
        # Record membrane potential
        self.dend_v = h.Vector()
        self.dend_v.record(self.dend(0.5)._ref_v)

    def data(self):
        return {"v": list(self.dend_v)}


## Creates half-gap junction mechanism
def mkgap(pc, sec, gid, secpos, sgid, dgid, w):
    myrank = int(pc.id())

    seg = sec(secpos)
    gj = h.ggap(seg)
    gj.g = w

    pc.source_var(seg._ref_v, sgid, sec=sec)
    pc.target_var(gj, gj._ref_vgap, dgid)

    if myrank == 0:
        print(
            "mkgap: gid %i: sec=%s sgid=%i dgid=%i w=%f"
            % (gid, str(sec), sgid, dgid, w)
        )
    return gj


class Model:
    def __init__(self, pc, ngids):
        self._cells = []
        self._gjlist = []
        self._stims = []
        self._make_cells(pc, ngids)
        self._make_gap_junctions(pc, ngids)
        # Record the time step values
        self._tvec = h.Vector()
        self._tvec.record(h._ref_t)

    def data(self):
        d = {str(cell): cell.data() for cell in self._cells}
        d["t"] = list(self._tvec)
        return d

    def _make_cells(self, pc, ngids):
        nranks = int(pc.nhost())
        myrank = int(pc.id())

        assert nranks <= ngids

        for gid in range(ngids):
            if gid % nranks == myrank:
                cell = Cell(gid)
                nc = h.NetCon(cell.soma(0.5)._ref_v, None, sec=cell.soma)
                pc.set_gid2node(gid, myrank)
                pc.cell(gid, nc, 1)
                self._cells.append(cell)

                # Current injection into section
                stim = h.IClamp(cell.soma(0.5))
                if gid % 2 == 0:
                    stim.delay = 10
                else:
                    stim.delay = 20
                stim.dur = 20
                stim.amp = 10
                self._stims.append(stim)

                if myrank == 0:
                    print(
                        "Rank %i: created gid %i; stim delay = %.02f"
                        % (myrank, gid, stim.delay)
                    )

    def _make_gap_junctions(self, pc, ngids):
        """Creates gap junctional connections:
        The first halfgap is created on even gids and gid 0 and the second halfgap is created on odd gids.
        """
        ggid = 2e6  ## gap junction id range is intended to not overlap with gid range
        for gid in range(0, ngids, 2):
            # source gid: all even gids
            src = gid
            # destination gid: all odd gids
            dst = gid + 1

            # source gap junction gid
            sgid = ggid
            # destination gap junction gid
            dgid = ggid + 1

            # if the source gid exists on this rank, create the half gap from src to dst
            if pc.gid_exists(src) > 0:
                cell = pc.gid2cell(src)
                sec = cell.dend
                self._gjlist.append(mkgap(pc, sec, gid, 0.5, sgid, dgid, 1.0))

            # if the destination gid exists on this rank, create the half gap from dst to src
            if pc.gid_exists(dst) > 0:
                cell = pc.gid2cell(dst)
                sec = cell.dend
                self._gjlist.append(mkgap(pc, sec, gid, 0.5, dgid, sgid, 1.0))

            ggid += 2


@pytest.mark.parametrize("enable_spt", [True, False])  # sparse parallel transfer
def test_par_gj(chk, enable_spt):
    """Test of ParallelTransfer-based gap junctions.
    Assumes the presence of a conductance-based half-gap junction model ggap.mod"""
    ngids = 2  # number of gids to create (must be even)
    with sparse_partrans(enable_spt), parallel_context() as pc, time_step(0.25):
        model = Model(pc, ngids)
        pc.setup_transfer()
        wt = time.time()
        pc.set_maxstep(10)
        h.finitialize(-65)
        pc.psolve(75)
        # compute time
        total_wt = time.time() - wt
        # parallel transfer time
        gjtime = pc.vtransfer_time()
        # model data
        model_data = model.data()
        del model
        # reference data
        ref_data = chk.get("test_par_gj", None)
        if ref_data is None:  # pragma: no cover
            # bootstrapping
            chk("test_par_gj", model_data)
        for key in model_data:
            assert model_data[key] == ref_data[key]

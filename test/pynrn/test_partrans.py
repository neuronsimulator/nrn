# ParallelContext transfer functionality tests.
# This uses nonsense models and values to verify transfer takes place correctly.

import sys
from io import StringIO

from neuron import h

# h.nrnmpi_init()

pc = h.ParallelContext()
rank = pc.id()
nhost = pc.nhost()

if nhost > 1:
    if rank == 0:
        print("nhost > 1 so calls to expect_error will return without testing.")


def expect_error(callable, args, sec=None):
    """
    Execute callable(args) and assert that it generated an error.

    If sec is not None, executes callable(args, sec=sec)
    Skips if nhost > 1 as all hoc_execerror end in MPI_ABORT
    Does not work well with nrniv launch since hoc_execerror messages do not
    pass through sys.stderr.
    """

    if nhost > 1:
        return

    old_stderr = sys.stderr
    sys.stderr = my_stderr = StringIO()
    err = 0
    try:
        if sec:
            callable(*args, sec=sec)
        else:
            callable(*args)
    except:
        err = 1
    errmes = my_stderr.getvalue()
    sys.stderr = old_stderr
    if errmes:
        errmes = errmes.splitlines()[0]
        errmes = errmes[(errmes.find(":") + 2) :]
        print("expect_error: %s" % errmes)
    if err == 0:
        print("expect_error: no err for %s%s" % (str(callable), str(args)))
    assert err


# HGap POINT_PROCESS via ChannelBUilder.
#  Cannot use with extracellular.
ks = h.KSChan(1)
ks.name("HGap")
ks.iv_type(0)
ks.gmax(0)
ks.erev(0)

# Cell with enough nonsense stuff to exercise transfer possibilities.
class Cell:
    def __init__(self):
        self.soma = h.Section(name="soma", cell=self)
        self.soma.diam = 10.0
        self.soma.L = 10.0
        self.soma.insert("na_ion")  # can use nai as transfer source
        # can use POINT_PROCESS range variable as targets
        self.ic = h.IClamp(self.soma(0.5))
        self.vc = h.SEClamp(self.soma(0.5))
        self.vc.rs = 1e9  # no voltage clamp current
        self.hgap = [None for _ in range(2)]  # filled by mkgaps


def run():
    pc.setup_transfer()
    h.finitialize()
    h.fadvance()


model = None  # Global allows teardown of model


def teardown():
    """
    destroy model
    """
    global model
    pc.gid_clear()
    model = None


def mkmodel(ncell):
    """
    Destroy existing model and re-create with ncell Cells.
    """
    global model
    if model:
        teardown()
    cells = {}
    for gid in range(rank, ncell, nhost):
        cells[gid] = Cell()
        pc.set_gid2node(gid, rank)
        pc.cell(gid, h.NetCon(cells[gid].soma(0.5)._ref_v, None, sec=cells[gid].soma))
    model = (cells, ncell)


def mkgaps(gids):
    """For list of gids, full gap, right to left"""
    gidset = set()
    for gid in gids:
        g = [gid, (gid + 1) % model[1]]
        sids = [i + 1000 for i in g]
        for i, j in enumerate([1, 0]):
            if pc.gid_exists(g[i]):
                cell = model[0][g[i]]
                if g[i] not in gidset:  # source var sid cannot be used twice
                    pc.source_var(cell.soma(0.5)._ref_v, sids[i], sec=cell.soma)
                    gidset.add(g[i])
                assert cell.hgap[j] is None
                cell.hgap[j] = h.HGap(cell.soma(0.5))
                pc.target_var(cell.hgap[j], cell.hgap[j]._ref_e, sids[j])
                cell.hgap[j].gmax = 0.0001


def transfer1(amp1=True):
    """
    round robin transfer v to ic.amp and vc.amp1, nai to vc.amp2
    """
    ncell = model[1]
    for gid, cell in model[0].items():
        s = cell.soma
        srcsid = gid
        tarsid = (gid + 1) % ncell
        pc.source_var(s(0.5)._ref_v, srcsid, sec=s)
        pc.source_var(s(0.5)._ref_nai, srcsid + ncell, sec=s)
        pc.target_var(cell.ic, cell.ic._ref_amp, tarsid)
        if amp1:
            pc.target_var(cell.vc, cell.vc._ref_amp1, tarsid)
        pc.target_var(cell.vc, cell.vc._ref_amp2, tarsid + ncell)


def init_values():
    """
    Initialize sources to their sid values and targets to 0
    This allows substantive test that source values make it to targets.
    """
    ncell = model[1]
    for gid, c in model[0].items():
        c.soma(0.5).v = gid
        c.soma(0.5).nai = gid + ncell
        c.ic.amp = 0
        c.vc.amp1 = 0
        c.vc.amp2 = 0


def check_values():
    """
    Verify that target values are equal to source values.
    """
    values = {}
    for gid, c in model[0].items():
        vi = c.soma(0.5).v
        if h.ismembrane("extracellular", sec=c.soma):
            vi += c.soma(0.5).vext[0]
        values[gid] = {
            "v": vi,
            "nai": c.soma(0.5).nai,
            "amp": c.ic.amp,
            "amp1": c.vc.amp1,
            "amp2": c.vc.amp2,
        }
    x = pc.py_gather(values, 0)
    if rank == 0:
        values = {}
        for v in x:
            values.update(v)
        ncell = len(values)
        for gid in values:
            v1 = values[gid]
            v2 = values[(gid + ncell - 1) % ncell]
            assert v1["v"] == v2["amp"]
            assert v1["v"] == v2["amp1"]
            assert v1["nai"] == v2["amp2"]


def test_partrans():

    # no transfer targets or sources.
    mkmodel(4)
    run()

    # invalid source or target sid.
    if 0 in model[0]:
        cell = model[0][0]
        s = cell.soma
        expect_error(pc.source_var, (s(0.5)._ref_v, -1), sec=s)
        expect_error(pc.target_var, (cell.ic, cell.ic._ref_amp, -1))

    # target with no source.
    if pc.gid_exists(1):
        cell = pc.gid2cell(1)
        pc.target_var(cell.ic, cell.ic._ref_amp, 1)
    expect_error(run, ())

    mkmodel(4)

    # source with no target (not an error).
    if pc.gid_exists(1):
        cell = pc.gid2cell(1)
        pc.source_var(cell.soma(0.5)._ref_v, 1, sec=cell.soma)
    run()

    # No point process for target
    if pc.gid_exists(1):
        cell = pc.gid2cell(1)
        pc.target_var(cell.vc._ref_amp3, 1)
    try:
        run()  # ok if test_fast_imem.py not prior
    except:
        pass
    pc.nthread(2)
    expect_error(run, ())  # Do not know the POINT_PROCESS target
    pc.nthread(1)

    # Wrong sec for source ref and wrong point process for target ref.
    mkmodel(1)
    if pc.gid_exists(0):
        cell = pc.gid2cell(0)
        sec = h.Section(name="dend")
        expect_error(pc.source_var, (cell.soma(0.5)._ref_v, 1), sec=sec)
        expect_error(pc.source_var, (cell.soma(0.5)._ref_nai, 2), sec=sec)
        del sec
        expect_error(pc.target_var, (cell.ic, cell.vc._ref_amp3, 1))
        # source sid already in use
        expect_error(pc.source_var, (cell.soma(0.5)._ref_nai, 1), sec=cell.soma)

    # partrans update: could not find parameter index

    # pv2node checks the parent
    mkmodel(1)
    s1 = h.Section(name="dend")
    s2 = h.Section(name="soma")
    ic = h.IClamp(s1(0.5))
    pc.source_var(s1(0)._ref_v, rank, sec=s1)
    pc.target_var(ic, ic._ref_amp, rank)
    run()
    assert s1(0).v == ic.amp
    """
  # but following changes the source node and things get screwed up
  # because of continuing to use a freed Node*. The solution is
  # beyond the scope of this pull request and would involve replacing
  # description in terms of Node* with (Section*, arc_position)
  s1.connect(s2(.5))
  run()
  print(s1(0).v, ic.amp)
  assert(s1(0).v == ic.amp)
  """
    # non_vsrc_update property disappears from Node*
    s1.insert("pas")  # not allowed to uninsert ions :(
    pc.source_var(s1(0.5)._ref_e_pas, rank + 10, sec=s1)
    pc.target_var(ic, ic._ref_delay, rank + 10)
    run()
    assert s1(0.5).e_pas == ic.delay
    s1.uninsert("pas")
    expect_error(run, ())
    teardown()
    del ic, s1, s2

    # missing setup_transfer
    mkmodel(4)
    transfer1()
    expect_error(h.finitialize, (-65,))

    # round robin transfer v to ic.amp and vc.amp1, nai to vc.amp2
    ncell = 5
    mkmodel(ncell)
    transfer1()
    init_values()
    run()
    check_values()

    # nrnmpi_int_alltoallv_sparse
    h.nrn_sparse_partrans = 1
    mkmodel(5)
    transfer1()
    init_values()
    run()
    check_values()
    h.nrn_sparse_partrans = 0

    # impedance error (number of gap junction not equal to number of pc.transfer_var)
    imp = h.Impedance()
    if 0 in model[0]:
        imp.loc(model[0][0].soma(0.5))
    expect_error(imp.compute, (1, 1))
    del imp

    # For impedance, pc.target_var requires that its first arg be a reference to the POINT_PROCESS"
    mkmodel(2)
    if pc.gid_exists(0):
        cell = pc.gid2cell(0)
        pc.source_var(cell.soma(0.5)._ref_v, 1000, sec=cell.soma)
        cell.hgap[1] = h.HGap(cell.soma(0.5))
        pc.target_var(cell.hgap[1], cell.hgap[1]._ref_e, 1001)
    if pc.gid_exists(1):
        cell = pc.gid2cell(1)
        pc.source_var(cell.soma(0.5)._ref_v, 1001, sec=cell.soma)
        cell.hgap[0] = h.HGap(cell.soma(0.5))
        pc.target_var(cell.hgap[0], cell.hgap[0]._ref_e, 1000)
    pc.setup_transfer()
    imp = h.Impedance()
    h.finitialize(-65)
    if pc.gid_exists(0):
        imp.loc(pc.gid2cell(0).soma(0.5))
    expect_error(imp.compute, (10, 1, 100))
    del imp, cell

    # impedance
    ncell = 5
    mkmodel(ncell)
    mkgaps(list(range(ncell - 1)))
    pc.setup_transfer()
    imp = h.Impedance()
    h.finitialize(-65)
    if 0 in model[0]:
        imp.loc(model[0][0].soma(0.5))
    niter = imp.compute(10, 1, 100)
    if rank == 0:
        print("impedance iterations=%d" % niter)
    # tickle execution of target_ptr_update for one more line of coverage.
    if 0 in model[0]:
        model[0][0].hgap[1].loc(model[0][0].soma(0))
        model[0][0].hgap[1].loc(model[0][0].soma(0.5))
    niter = imp.compute(10, 1, 100)
    del imp

    # CoreNEURON gap file generation
    mkmodel(ncell)
    transfer1()

    # following is a bit tricky and need some user help in the docs.
    cvode = h.CVode()
    assert cvode.use_mxb(0) == 0

    pc.setup_transfer()
    h.finitialize(-65)
    pc.nrncore_write("tmp")

    # CoreNEURON: one thread empty of gaps
    mkmodel(1)
    transfer1()
    s = h.Section("dend")
    pc.set_gid2node(rank + 10, rank)
    pc.cell(rank + 10, h.NetCon(s(0.5)._ref_v, None, sec=s))
    pc.nthread(2)
    pc.setup_transfer()
    h.finitialize(-65)
    pc.nrncore_write("tmp")
    pc.nthread(1)
    teardown()
    del s

    # There used to be single thread circumstances where target POINT_PROCESS is needed
    # With the new data_handle scheme, the pointer update that used to need the target
    # POINT_PROCESS is no longer made.
    s = h.Section("dend")
    pc.set_gid2node(rank, rank)
    pc.cell(rank, h.NetCon(s(0.5)._ref_v, None, sec=s))
    pc.source_var(s(0.5)._ref_v, rank, sec=s)
    ic = h.IClamp(s(0.5))
    pc.target_var(ic._ref_amp, rank)
    pc.setup_transfer()
    h.finitialize(-65)
    teardown()
    del ic, s

    # threads
    mkmodel(ncell)
    transfer1()
    pc.nthread(2)
    init_values()
    run()
    check_values()
    pc.nthread(1)

    # extracellular means use v = vm+vext[0]
    for cell in model[0].values():
        cell.soma.insert("extracellular")
    init_values()
    run()
    check_values()

    teardown()


if __name__ == "__main__":
    test_partrans()
    pc.barrier()
    h.quit()

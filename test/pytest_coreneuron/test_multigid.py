# Support multiple output gids per cell in coreneuron via
# pc.cell(gid, h.NetCon(cell.sec(x)._ref_v, None, sec=cell.sec))
# It is an error to have multiple gids with the same reference.

from neuron import config, h, coreneuron
from neuron.expect_hocerr import expect_err
from math import log10


pc = h.ParallelContext()


def coreneuron_available():
    if not config.arguments["NRN_ENABLE_CORENEURON"]:
        return False
    # But can it be loaded?
    cvode = h.CVode()
    pc = h.ParallelContext()
    h.finitialize()
    result = 0
    import sys
    from io import StringIO

    original_stderr = sys.stderr
    sys.stderr = StringIO()
    try:
        pc.nrncore_run("--tstop 1 --verbose 0")
        result = 1
    except Exception as e:
        pass
    sys.stderr = original_stderr
    return result


cn_avail = coreneuron_available()


class Cell:
    def __init__(self, id):
        self.id = id
        self.axon = a = h.Section(name="axon", cell=self)
        a.L = 1000
        a.diam = 1
        a.nseg = 5
        a.insert("hh")
        self.ic = ic = h.IClamp(a(0))
        ic.delay = id / 10.0
        ic.dur = 0.3
        ic.amp = 0.4

    def __str__(self):
        return "Cell_" + str(self.id)


def pccell(gid, seg):
    pc.set_gid2node(gid, pc.id())
    pc.cell(gid, h.NetCon(seg._ref_v, None, sec=seg.sec))
    pc.threshold(gid, -10.0)


class Net:
    def __init__(self, ncell):
        gidfac = 10 ** (int(log10(ncell)) + 1)
        pc.gid_clear()
        self.cells = {i: Cell(i) for i in range(pc.id(), ncell, pc.nhost())}
        for i, cell in self.cells.items():
            # a gid in every segment
            for iseg, seg in enumerate(cell.axon.allseg()):
                gid = i + gidfac * iseg
                pccell(gid, seg)

        self.raster = [h.Vector(), h.Vector()]
        pc.spike_record(-1, self.raster[0], self.raster[1])


def run(tstop):
    pc.set_maxstep(10)
    h.finitialize(-65)
    pc.psolve(tstop)


def praster(r):
    print("praster")
    for i in range(r[0].size()):
        print(r[0][i], r[1][i])


def raster_eq(r1, r2):
    for i in range(2):
        assert r1[i].eq(r2[i])


def test_multigid():
    net = Net(3)
    run(10.0)
    std = [net.raster[0].c(), net.raster[1].c()]
    assert std[0].size() == 21

    if cn_avail:
        coreneuron.enable = True
        coreneuron.verbose = 0
        run(10.0)
        raster_eq(std, net.raster)
        coreneuron.enable = False
        print("test_multigid coreneuron success")

    s = None
    if pc.id() == 0:
        expect_err("pccell(10001, net.cells[0].axon(.5))")
        pc.set_gid2node(10002, pc.id())
        expect_err("pc.cell(10002, h.NetCon(None, None))")
        s = h.Section("soma", net.cells[0])
        pc.cell(10002, h.NetCon(s(0.5)._ref_v, None, sec=s), 0)  # line of coverage

        if cn_avail:
            s.insert("hh")
            pc.set_gid2node(10003, pc.id())
            pc.cell(10003, h.NetCon(s(0.5).hh._ref_m, None, sec=s), 0)
            coreneuron.enable = True
            expect_err("run(10)")
            coreneuron.enable = False

    pc.gid_clear()
    del s, net, std
    locals()


def test_nogid():
    net = Net(2)
    s = h.Section("soma", net.cells[0])
    syn = h.ExpSyn(net.cells[1].axon(0.5))
    nc = h.NetCon(s(0.5)._ref_v, syn, sec=s)
    if cn_avail:
        coreneuron.enable = True
        expect_err("run(10)")
        coreneuron.enable = False
    pc.gid_clear()
    del nc, syn, s, net
    locals()


if __name__ == "__main__":
    test_multigid()
    test_nogid()

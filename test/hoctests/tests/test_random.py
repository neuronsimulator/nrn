from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet
from neuron.tests.utils.checkresult import Chk
import os
import math

pc = h.ParallelContext()

dir_path = os.path.dirname(os.path.realpath(__file__))
chk = Chk(os.path.join(dir_path, "test_random.json"))

set_quiet(False)

z = h.List("NMODLRandom")

# ARTIFICIAL_CELL syntax tests
rt = h.RanArt()

print(rt.ran1)
assert z.count() == 0
assert rt.ran1.set_seq(5).set_ids(7, 8, 9).get_seq() == 5
assert rt.ran1.get_ids().x[2] == 9
assert z.count() == 0

x = rt.ran1
x.set_seq(25)
assert rt.ran1.get_seq() == 25
assert z.count() == 1
assert x.get_ids().x[1] == 8

y = rt.ran1.set_seq
y(50)
assert x.get_seq() == 50

del x, y
assert z.count() == 0

x = rt.ran1
expect_err("rt.ran1 = rt.ran2")  # cannot assign
del rt
expect_err("x.get_seq()")
del x
assert z.count() == 0

# density mechanism tests
cable = h.Section(name="cable")
cable.nseg = 3
cable.insert("rantst")
nr = [(seg.x, seg.rantst.ran1) for seg in cable]
for r in nr:
    assert r[1].get_ids().eq(cable(r[0]).ran1_rantst.get_ids())
del nr, r
assert z.count() == 0

rt = h.RanArt()
r1 = rt.ran1
# wrap around at end
assert r1.set_seq(2**34 - 1).get_seq() == (2**34 - 1)
r1.uniform()
assert r1.get_seq() == 0
assert r1.set_seq(2**34 + 1).get_seq() == 1
assert r1.set_seq(-10).get_seq() == 0  # all neg setseq to 0
assert r1.set_seq(2**40 - 1).get_seq() == 17179869183.0  # up to 2**40 wrap
assert r1.set_seq(2**40 + 1).get_seq() == 0  # all above 2**40 setseq to 0

# negexp(mean) has proper scale
r1.set_seq(0)
x = rt.negexp0()
r1.set_seq(0)
assert math.isclose(rt.negexp1(5), 5 * x)

r1 = h.NMODLRandom()
expect_err("r1.uniform()")
del r1

# test the mod random_... functions
rt = h.RanArt()
cable = h.Section(name="cable")
cable.nseg = 3
cable.insert("rantst")
mlist = [rt]
mlist.extend(seg.rantst for seg in cable)
for i, m in enumerate(mlist):
    m.ran1.set_ids(i, 1, 0).set_seq(0)
    m.ran2.set_ids(i, 2, 0).set_seq(0)

results = []
for m in mlist:
    results.extend([m.uniform0(), m.negexp0(), m.normal0()])
    results.extend(
        [
            m.uniform2(10, 20),
            m.negexp1(5),
            m.normal2(5, 8),
        ]
    )
    results.extend([m.ran1.uniform(), m.ran2.uniform()])
for m in mlist:
    results.extend([m.ran1.get_seq(), m.ran2.get_seq()])

assert z.count() == 0

chk("results", results, tol=1e-12)

# test access to hoc cell class
h(
    """
begintemplate Cell
public cable, rt
create cable
objref rpp, rart
proc init() {
  create cable
  cable {
    nseg = 3
    insert rantst
    rpp = new RanPP(0.1) 
  }
  rart = new RanArt()
  rart.mean = 0.1
}
endtemplate Cell
"""
)
cell = h.Cell()
cell.cable(0.1).ran2_rantst.set_seq(10).set_ids(8, 9, 10)
cell.cable(0.5).ran2_rantst.set_seq(11).set_ids(1, 2, 3)
assert cell.cable(0.1).rantst.ran2.get_seq() == 10

rp = cell.rpp
del cell
expect_err("rp.ran2")


def set_gids(cells):
    gid = 0
    for cell in cells:
        for port in [cell.cable(0.5)._ref_v, cell.rart]:
            gid += 1
            pc.set_gid2node(gid, pc.id())
            pc.cell(gid, h.NetCon(port, None, sec=cell.cable))


def test_bbsavestate():
    cell = h.Cell()
    set_gids([cell])  ## BBSaveState requires that real cells have gids
    mechs = [cell.cable(0.5).rantst, cell.rpp, cell.rart]
    rec = []
    for i, m in enumerate(mechs):
        m.ran2.set_ids(i, 2, 3)
        rec.append(h.Vector().record(m._ref_x2, sec=cell.cable))

    def run(tstop, init=True):
        if init:
            pc.set_maxstep(10)
            h.finitialize(-65)
        else:
            h.frecord_init()
        pc.psolve(tstop)

    # rerunning in two parts gives same result.
    run(1)
    bbss = h.BBSaveState()
    bbss.save("bbss_random.txt")

    # second half is our standard
    run(2, False)
    chk("bbsavestate 1 to 2", [v.to_python() for v in rec], tol=1e-12)

    # savestate restore and redo second half.
    bbss.restore("bbss_random.txt")
    run(2, False)
    chk("bbsavestate 1 to 2", [v.to_python() for v in rec], tol=1e-12)


test_bbsavestate()

chk.save()

from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet
from neuron.tests.utils.checkresult import Chk
import os

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
assert r1.set_seq(2**34).get_seq() == 0

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

chk("results", results)

# test access to hoc cell class
h(
    """
begintemplate Cell
public cable, rt
create cable
objref rt  
proc init() {
  create cable
  cable {
    nseg = 3
    insert rantst
  }
  rt = new RanPP() 
}
endtemplate Cell
"""
)
cell = h.Cell()
cell.cable(0.1).ran2_rantst.set_seq(10).set_ids(8, 9, 10)
cell.cable(0.5).ran2_rantst.set_seq(11).set_ids(1, 2, 3)
assert cell.cable(0.1).rantst.ran2.get_seq() == 10

rp = cell.rt
del cell
expect_err("rp.ran2")

chk.save()

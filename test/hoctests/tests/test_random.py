from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)

z = h.List("NMODLRandom")

# ARTIFICIAL_CELL syntax tests
rt = h.RanTst()

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

rt = h.RanTst()
r1 = rt.ran1
# wrap around at end
assert r1.set_seq(2**34 - 1).get_seq() == (2**34 - 1)
r1.uniform()
assert r1.get_seq() == 0
assert r1.set_seq(2**34).get_seq() == 0

r1 = h.NMODLRandom()
expect_err("r1.uniform()")
del r1

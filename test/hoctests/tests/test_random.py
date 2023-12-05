from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)

z = h.List("NMODLRandom")

# ARTIFICIAL_CELL syntax tests
ns = h.NetStimRNG()

print(ns.rrr)
assert z.count() == 0
assert ns.rrr.set_seq(5).set_ids(7, 8, 9).get_seq() == 5
assert ns.rrr.get_ids().x[2] == 9
assert z.count() == 0

x = ns.rrr
x.set_seq(25)
assert ns.rrr.get_seq() == 25
assert z.count() == 1
assert x.get_ids().x[1] == 8

y = ns.rrr.set_seq
y(50.0)
assert x.get_seq() == 50.0

del x, y
assert z.count() == 0

x = ns.rrr
del ns
expect_err("x.get_seq()")
del x
assert z.count() == 0

# density mechanism tests
cable = h.Section(name="cable")
nseg = 3
cable.insert("rantst")
nr = [seg.rantst.rrr for seg in cable]
for r in nr:
    print(r.get_ids().to_python())

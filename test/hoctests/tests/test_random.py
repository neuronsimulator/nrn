from neuron import h

z = h.List("NMODLRandom")

# POINT_PROCESS syntax tests
ns = h.NetStimRNG()
h.finitialize()

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

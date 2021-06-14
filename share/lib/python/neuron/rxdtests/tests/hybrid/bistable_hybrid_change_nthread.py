from neuron import h, crxd as rxd
import numpy
import __main__

name = __main__.__file__
if name[-3:] == ".py":
    name = name[:-3]

h.load_file("stdrun.hoc")

dend1 = h.Section()
dend1.diam = 2
dend1.nseg = 101
dend1.L = 50

dend2 = h.Section()
dend2.diam = 2
dend2.nseg = 101
dend2.L = 50
dend2.connect(dend1)

diff_constant = 1

r = rxd.Region(h.allsec(), dx=0.25)

rxd.set_solve_type([dend1], dimension=3)

ca = rxd.Species(
    r,
    d=diff_constant,
    initial=lambda node: 1
    if (0.8 < node.x and node.segment in dend1)
    or (node.x < 0.2 and node.segment in dend2)
    else 0,
)
bistable_reaction = rxd.Rate(ca, -ca * (1 - ca) * (0.01 - ca))
h.finitialize()

for i in range(2):
    h.fadvance()
rxd.nthread(2)
for i in range(2, 4):
    h.fadvance()
rxd.nthread(3)
for i in range(4, 6):
    h.fadvance()
rxd.nthread(4)
for i in range(6, 8):
    h.fadvance()
rxd.nthread(1)
for i in range(8, 11):
    h.fadvance()

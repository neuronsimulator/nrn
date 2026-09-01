from neuron import h
from math import isclose
import os


class Cell:
    def __init__(self, id):
        self.id = id

    def __str__(self):
        return "Cell_%d" % self.id


cell = Cell(0)
h.load_file("stdrun.hoc")
h.load_file("import3d.hoc")
nl = h.Import3d_Neurolucida3()
nl.quiet = 1
dir_path = os.path.dirname(os.path.realpath(__file__))
fname = os.path.join(dir_path, "test_pt3dstyle.asc")
nl.input(fname)
import_neuron = h.Import3d_GUI(nl, 0)
import_neuron.instantiate(cell)


def logcon(s):
    lcf = tuple([h.ref(0.0) for _ in range(3)])
    lc = h.pt3dstyle(1, *lcf, sec=s)
    return lc, [ref[0] for ref in lcf]


def vec3d(s):
    vecs = [[] for _ in range(4)]  # x, y ,z, d
    for i, f in enumerate([h.x3d, h.y3d, h.z3d, h.diam3d]):
        for j in range(s.n3d()):
            vecs[i].append(f(j))
    return vecs


def allpts():
    return [vec3d(s) for s in h.allsec()]


stdpts = allpts()
stdlogcon = logcon(cell.axon[0])

h.define_shape()

assert stdpts == allpts()
# The logical connection is only accurate to float32 resolution
print(stdlogcon)
print(logcon(cell.axon[0]))
# assert stdlogcon == logcon(cell.axon[0])
for i, val in enumerate(logcon(cell.axon[0])[1]):
    assert isclose(stdlogcon[1][i], val, rel_tol=1e-7)

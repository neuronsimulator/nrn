"""Repeating a CoreNEURON simulation gives the same result, bit for bit, also on a GPU.

On the GPU the axial terms of the cable equation used to be added to each node
with atomic updates, in an order that could change from one run to the next.
Branched cells show it: a branch point receives terms from several children.
"""

from neuron.tests.utils.strtobool import strtobool
import os

from neuron import h

pc = h.ParallelContext()


class Cell:
    """An hh soma with dendritic trees at both of its ends."""

    def __init__(self, i):
        self.soma = h.Section(name=f"soma_{i}")
        self.soma.L = self.soma.diam = 20
        self.soma.insert("hh")
        self.dends = []
        for k, x in enumerate([1, 1, 1, 0]):
            self.tree(i, self.soma, x, 1, 2.0 - 0.2 * k)
        self.ic = h.IClamp(self.soma(0.5))
        self.ic.delay, self.ic.dur, self.ic.amp = 1, 100, 0.4 + 0.1 * (i % 5)
        pc.set_gid2node(i, pc.id())
        self.nc = h.NetCon(self.soma(0.5)._ref_v, None, sec=self.soma)
        pc.cell(i, self.nc)

    def tree(self, i, parent, x, depth, diam):
        s = h.Section(name=f"dend_{i}_{len(self.dends)}")
        s.L, s.diam, s.nseg = 80 + 10 * (len(self.dends) % 4), diam, 3 + i % 3
        s.insert("hh")
        s.connect(parent(x))
        self.dends.append(s)
        if depth < 3:
            self.tree(i, s, 1, depth + 1, diam * 0.7)
            self.tree(i, s, 1, depth + 1, diam * 0.6)


def test_axial_order():
    cells = [Cell(i) for i in range(10)]
    vecs = [h.Vector().record(c.soma(0.5)._ref_v) for c in cells]

    from neuron import coreneuron

    coreneuron.enable = True
    coreneuron.verbose = 0
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))

    def run():
        pc.set_maxstep(10)
        h.finitialize(-65)
        pc.psolve(50)
        return [v.to_python() for v in vecs]

    first = run()
    for _ in range(3):
        assert run() == first
    coreneuron.enable = False
    pc.gid_clear()


if __name__ == "__main__":
    test_axial_order()

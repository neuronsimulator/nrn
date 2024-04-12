from neuron import h
from neuron.tests.utils.checkresult import Chk
import os

# Create a helper for managing reference results
dir_path = os.path.dirname(os.path.realpath(__file__))
chk = Chk(os.path.join(dir_path, "test_optim_node_order.json"))

pc = h.ParallelContext()

# Default Node order for simulation given a particular construction order
# is the order of section creation.


class Cell:
    def __init__(self, id):
        self.id = id
        d = self.den = h.Section(name="d", cell=self)
        d.nseg = id
        for i, seg in enumerate(d.allseg()):
            seg.v = i + 10 * id

        pc.set_gid2node(id, pc.id())
        pc.cell(id, h.NetCon(d(1)._ref_v, None, sec=d))
        syns = self.syns = [h.ExpSyn(d(0)) for i in range(3)]
        for i, syn in enumerate(syns):
            syn.tau = i + 10 * id

    def __str__(self):
        return "Cell_%d" % self.id


cells = [Cell(id) for id in range(1, 4)]

pr = False


def printv():
    s = {}  # associate root sections with thread id.
    for i in range(pc.nthread()):
        for sec in pc.get_partition(i):
            s[sec] = i
    if pr:
        print(s)
    ns = 0
    for sec in h.allsec():
        for seg in sec.allseg():
            if pr:
                print(
                    seg,
                    seg.v,
                    seg.node_index(),
                    seg._ref_v,
                    str(s[sec] if sec in s else ""),
                )
            ns += 1

    names = [None for i in range(ns)]
    for sec in h.allsec():
        for seg in sec.allseg():
            ix = int(str(seg._ref_v).split()[1][4:].split("/")[0])
            names[ix] = seg.v
    for i, seg in enumerate(names):
        if pr:
            print(i, seg)
    tag = "node order %d   nthread %d" % (pc.optimize_node_order(), pc.nthread())
    chk(tag, names)


# printv()

# Iteration order (sec,seg) is associated with construction order
# (provided nseg set after each section construction?)
# and the only difference in simulation order is that all root nodes are at the beginning.
# (provided there is only one thread)


def pvmes(i):
    if pr:
        print("node order %d   nthread %d" % (i, pc.nthread()))
    h.finitialize()
    printv()


def p():
    for i in range(3):
        pc.optimize_node_order(i)
        pvmes(i)


p()
pc.nthread(2)
p()

chk.save()

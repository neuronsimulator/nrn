from neuron import h

h.load_file("stdrun.hoc")


class Cell(object):
    def __init__(self):
        self.topology()
        self.subsets()
        self.geometry()
        self.biophys()
        self.synapses()

    def topology(self):
        self.soma = h.Section(cell=self)
        self.dend = h.Section(cell=self)
        self.dend.connect(self.soma)
        self.nseg = 5

    def subsets(self):
        self.all = h.SectionList()
        self.all.wholetree(sec=self.soma)

    def geometry(self):
        self.soma.L = 10
        self.soma.diam = 10
        self.dend.L = 500
        self.dend.diam = 1

    def biophys(self):
        for sec in self.all:
            sec.Ra = 100
            sec.cm = 1
        self.soma.insert("hh")

    def synapses(self):
        self.syn = h.ExpSyn(0.5, sec=self.dend)
        self.syn.e = 0
        self.syn.tau = 1

    def connectToTarget(self, syn):
        nc = h.NetCon(self.soma(0.5)._ref_v, syn, sec=self.soma)
        nc.threshold = -10
        return nc


# read which group we are part of
# in the old c interface it was
# g = new_intp()
# if MUSIC_configInt(s, 'group', g):
#  group = intp_value(g)
#  delete_intp(g)
# else:
#  group = 0
# del(g)
from sys import argv
from getopt import gnu_getopt

optlist, args = gnu_getopt(argv[3:], "g:")
group = int(optlist[0][1])

pc = h.ParallelContext()
rank = int(pc.id())
print("group=", group, " rank=", rank)

gid = group
pc.set_gid2node(gid, pc.id())
cell = Cell()
pc.cell(gid, cell.connectToTarget(None))
if gid == 1:
    stim = h.IClamp(cell.soma(0.5))
    stim.delay = 0
    stim.dur = 0.5
    stim.amp = 0.2


from neuronmusic import *

out = publishEventOutput("output")
inp = publishEventInput("input")

out.gid2index(gid, gid - 1)
nc = inp.index2target(gid % 2, cell.syn)
nc.weight[0] = 0.005
nc.delay = 8

pc.set_maxstep(5)
print(group, rank, " call stdinit")
h.stdinit()
print(group, rank, " call psolve")
pc.psolve(50)
print(group, rank, " done")

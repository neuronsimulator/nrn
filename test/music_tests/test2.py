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


pc = h.ParallelContext()
pc.set_gid2node(1, pc.id())
cell = Cell()
pc.cell(1, cell.connectToTarget(None))
stim = h.IClamp(cell.soma(0.5))
stim.delay = 0
stim.dur = 15
stim.amp = 0.1


from neuronmusic import *

out = publishEventOutput("out")
out.gid2index(1, 0)

pc.set_maxstep(1)
h.stdinit()
pc.psolve(20)

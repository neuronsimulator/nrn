from neuron import h
import math


class Cell:
    def __init__(self):
        soma = h.Section(name="soma", cell=self)
        dend = h.Section(name="dend", cell=self)
        dend.connect(soma)

        soma.nseg = 1
        soma.L = 10
        soma.diam = 100.0 / soma.L / math.pi
        soma.insert("hh")

        dend.nseg = 25
        dend.L = 1000
        dend.diam = 2
        dend.insert("hh")
        for seg in dend:
            seg.hh.gnabar /= 2
            seg.hh.gkbar /= 2
            seg.hh.gl /= 2

        self.dend = dend
        self.soma = soma
        for sec in [soma, dend]:
            sec.Ra = 100
            sec.cm = 1

from neuron import h

pc = h.ParallelContext()


class PyCell:
    def __init__(self, gid):
        s = {i: h.Section(name=i, cell=self) for i in ["soma", "dend", "axon"]}
        s["dend"].connect(s["soma"](0))
        s["axon"].connect(s["soma"](1))
        self.soma = s["soma"]
        a = s["axon"]
        self.sections = s
        for s in self.sections.values():
            s.L = 10
            s.diam = 10 if "soma" in s.name() else 1
            s.insert("hh")

        self.syn = h.ExpSyn(self.sections["dend"](0.5))
        self.syn.e = 0
        self.gid = gid
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(a(1)._ref_v, None, sec=a))


h(
    """
objref pc
pc = new ParallelContext()
begintemplate TestHocPOCell
  public soma, dend, axon, gid, syn
  external pc
  create soma, dend, axon
  objref syn
  proc init() { localobj nc, nil
    connect dend(0), soma(0)
    connect axon(0), soma(1)
    forall { insert hh  L=10  diam = 1 }
    soma.diam = 10

    dend {syn = new ExpSyn(.5)}
    syn.e = 0
    gid = $1
    pc.set_gid2node(gid, pc.id())
    axon nc = new NetCon(&v(1), nil)
    pc.cell(gid, nc)
  }
endtemplate TestHocPOCell
"""
)


class Ring:
    def __init__(self, npy, nhoc):
        pc.gid_clear()
        self.ngid = npy + nhoc
        self.cells = {gid: PyCell(gid) for gid in range(npy)}
        self.cells.update({gid: h.TestHocPOCell(gid) for gid in range(npy, self.ngid)})
        self.mkcon(self.ngid)
        self.mkstim()
        self.spiketime = h.Vector()
        self.spikegid = h.Vector()
        pc.spike_record(-1, self.spiketime, self.spikegid)
        for gid in range(self.ngid):
            pc.threshold(gid, -10.0)

    def mkcon(self, ngid):
        self.ncs = [
            pc.gid_connect(gid, self.cells[(gid + 1) % ngid].syn) for gid in range(ngid)
        ]
        for nc in self.ncs:
            nc.delay = 1.0
            nc.weight[0] = 0.004

    def mkstim(self):
        self.ic = h.IClamp(self.cells[0].soma(0.5))
        self.ic.delay = 1
        self.ic.dur = 0.2
        self.ic.amp = 0.3

from neuron import h
from neuron.expect_hocerr import expect_err
from checkresult import Chk

import os

datadir = "test/cover/" if os.path.isdir("test/cover") else "./"
chk = Chk(datadir + "test_netcvode.json")

cv = h.CVode()
pc = h.ParallelContext()

# helpful for debugging when expect no NetCon exist
def prNetCon(s):
    print(s)
    ncl = h.List("NetCon")
    print("NetCon count ", len(ncl))
    for nc in ncl:
        print("    ", nc, nc.pre(), nc.preseg(), nc.syn())


# helpful for debugging when expect no Sections
def prSection(s):
    print(s)
    nsec = sum(1 for _ in h.allsec())
    print("Section count ", nsec)
    for sec in h.allsec():
        print("    ", sec.name())


# call netcvode.cpp: void nrn_use_daspk(int b)
def nrn_use_daspk():
    cv.use_daspk(0)
    s = h.Section()
    cv.active(1)
    s.insert("extracellular")
    h.finitialize(-65)
    s.diam = 200
    h.finitialize(-65)
    assert cv.use_daspk() == 1.0
    del s
    cv.use_daspk(0)
    cv.active(0)


# call netcvode: static Node*(Object*)
def node():
    # Requires source POINT_PROCESS with "x" threshold variable and lvardt
    # Such a POINT_PROCESS is discouraged in favor of NET_RECEIVE with WATCH.
    s = h.Section()
    s.L = 10
    s.diam = 10
    cv.active(1)
    ic = h.IClamp(s(0.5))
    ic.delay = 0
    ic.dur = 5
    ic.amp = 0.3
    src = h.PPxSrc(s(0.5))
    nc = h.NetCon(src, None)

    results = [
        [
            (0.6500000000999997, 12.070427805842268),
            (0.7000315963318277, 16.84809332604498),
            (0.7000315963318277, 16.84809332604498),
        ],
        [
            (0.6500000000999997, 12.070427805842268),
            (0.6283185307179584, 10.0),
            (0.6283185307179584, 10.0),
        ],
    ]

    def ev(*arg):
        print("ev t=%g v=%g x=%g nc.x=%g" % (h.t, s(0.5).v, src.x, nc.x))
        assert (h.t, src.x) == results[arg[0]][arg[1]]

    def run():
        order = 0
        h.dt = 0.025
        type = (
            "fixed" if cv.active() == 0 else "lvardt" if cv.use_local_dt() else "cvode"
        )
        method = 0 if cv.active() == 0 else 2 if cv.use_local_dt() else 1
        if cv.active():
            type += " condition_order %d" % cv.condition_order()
            order = 0 if cv.condition_order() == 1 else 1
        nc.record((ev, (order, method)))
        print("\n" + type, "  thresh ", nc.threshold)
        pc.set_maxstep(10)
        h.finitialize(-50)
        pc.psolve(1)

    def series(condorder):
        cv.condition_order(condorder)
        cv.active(0)
        run()
        cv.active(1)
        run()
        cv.use_local_dt(1)
        run()
        cv.use_local_dt(0)
        cv.active(0)

    # otherwise, the first order threshold crossing is missed
    cv.maxstep(0.1)
    series(1)
    cv.maxstep(1000)
    series(2)
    del nc


def p(s):
    print(s)
    ncl = h.List("NetCon")
    print("NetCon count ", len(ncl))
    for nc in ncl:
        print("    ", nc, nc.pre(), nc.preseg(), nc.syn())


class Cell:
    def __init__(self, id):
        self.id = id
        self.soma = h.Section(name="soma", cell=self)
        self.soma.diam = 10.0
        self.soma.L = 10.0
        self.soma.insert("hh")

    def __str__(self):
        return "Cell" + str(self.id)


h(
    """ // HOC cells work best for CVode.netconlist
begintemplate HCell
public soma
create soma
proc init() {
  create soma
  soma {
    diam = 10
    L = 10
    insert hh
  }
}
endtemplate HCell
"""
)


class Net:
    # all to all
    def __init__(self, ncell, hcell=False):
        cells = (
            [Cell(i) for i in range(ncell)]
            if hcell == False
            else [h.HCell() for _ in range(ncell)]
        )
        syns = {}  # {(i,j):(ExpSyn,NetCon)}
        for i in range(ncell):
            for j in range(ncell):
                syn = h.ExpSyn(cells[j].soma(0.5))
                # spikes via inhibitory rebound and synchronizes
                syn.e = -120
                syn.tau = 3
                nc = h.NetCon(cells[i].soma(0.5)._ref_v, syn, sec=cells[i].soma)
                nc.delay = 5
                nc.weight[0] = 0.001
                nc.threshold = -10
                syns[(i, j)] = (syn, nc)
        ic = h.IClamp(cells[0].soma(0.5))
        ic.delay = 0
        ic.dur = 0.1
        ic.amp = 0.3
        self.cells = cells
        self.syns = syns
        self.ic = ic


def netcon_access():
    net = Net(4)

    # NetCon.preloc and NetCon.preseg
    x = net.syns[(0, 1)][1].preloc()
    sec = h.cas()
    h.pop_section()
    assert sec(x) == net.cells[0].soma(0.5)
    assert net.syns[(0, 1)][1].preseg() == net.cells[0].soma(0.5)
    # two more lines of coverage if source is at 0 loc of section
    nc = h.NetCon(
        net.cells[0].soma(0)._ref_v, net.syns[(0, 1)][0], sec=net.cells[0].soma
    )
    x = nc.preloc()
    sec = h.cas()
    h.pop_section()
    assert sec(x) == net.cells[0].soma(0)
    assert nc.preseg() == net.cells[0].soma(0)
    del nc
    # if NetCon source not a section then return -1 or None
    acell = h.NetStim()
    nc = h.NetCon(acell, net.syns[(0, 1)][0])
    x = nc.preloc()
    assert x == -1.0  # and no need to pop the section
    assert nc.preseg() == None
    del nc, acell
    # if NetCon source not a voltage but in a section then return -2 or None
    nc = h.NetCon(
        net.cells[0].soma(0.5).hh._ref_m, net.syns[(0, 1)][0], sec=net.cells[0].soma
    )
    x = nc.preloc()
    assert x == -2.0
    h.pop_section()
    assert nc.preseg() == None
    del nc

    # NetCon.postloc, NetCon.postseg, NetCon.syn, NetCon.pre
    x = net.syns[(0, 1)][1].postloc()
    sec = h.cas()
    h.pop_section()
    assert sec(x) == net.cells[1].soma(0.5)
    assert net.syns[(0, 1)][1].postseg() == net.cells[1].soma(0.5)
    assert net.syns[(0, 1)][1].pre() == None
    # if NetCon.target not associated with a section then return -1 or None
    a = [h.IntFire1() for _ in range(2)]
    nc = h.NetCon(a[0], a[1])
    x = nc.postloc()
    assert x == -1.0
    assert nc.postseg() == None
    assert nc.pre() == a[0]
    assert nc.syn() == a[1]
    nc = h.NetCon(a[0], None)
    assert nc.syn() == None
    del nc, a, x, sec

    # NetCon.synlist, NetCon.prelist, NetCon.postcelllist, NetCon.precelllist
    # NetCon.precell, NetCon.postcell NetCon.wcnt
    a = [h.IntFire1() for _ in range(2)]
    nclist = [h.NetCon(a[0], a[1]) for _ in range(10)]
    nclist2 = nclist[0].synlist()
    assert nclist2.count() == 10
    nclist2 = h.List()
    nclist[0].synlist(nclist2)
    assert nclist2.count() == 10
    nclist2 = nclist[0].prelist()
    assert nclist2.count() == 10
    net = Net(4, True)  # til fix nrn_sec2cell memory leak
    nclist2 = net.syns[(0, 1)][1].prelist()
    assert nclist2.count() == len(net.cells)
    nclist2 = net.syns[(0, 1)][1].precelllist()
    assert nclist2.count() == len(net.cells)
    nclist2 = net.syns[(0, 1)][1].postcelllist()
    assert nclist2.count() == len(net.cells)
    nclist2 = nclist[0].postcelllist()  # works only for Real cells
    assert nclist2.count() == 0
    assert nclist[0].precell() == None
    assert nclist[0].postcell() == None
    assert net.syns[(2, 3)][1].precell() == net.cells[2]
    assert net.syns[(2, 3)][1].postcell() == net.cells[3]
    del nclist, nclist2, a
    # NetCon.setpost
    net.syns[(0, 1)][1].setpost(None)
    assert net.syns[(0, 1)][1].postseg() == None
    expect_err("net.syns[(0, 1)][1].setpost([])")
    net.syns[(0, 1)][1].setpost(net.syns[(0, 1)][0])
    assert net.syns[(0, 1)][1].syn() == net.syns[(0, 1)][0]
    # added a setpost test in coreneuron/test_fornetcon.py to cover the
    # case of a change in weight vector size.

    # NetCon.valid NetCon.active
    net = Net(2)
    assert net.syns[(0, 1)][1].active()
    assert net.syns[(0, 1)][1].active(0) == True
    assert net.syns[(0, 1)][1].active() == False
    assert net.syns[(0, 1)][1].active(1) == False
    assert net.syns[(0, 1)][1].valid()
    net.syns[(0, 1)][1].setpost(None)
    assert net.syns[(0, 1)][1].valid() == False

    # NetCon.event
    net = Net(2)
    net.ic.amp = 0.0  # no intrinsic activity
    pc.set_maxstep(10)
    h.finitialize(-65)
    net.syns[(0, 1)][1].active(0)
    assert net.syns[(0, 1)][1].event(0.1) == 0
    net.syns[(0, 1)][1].active(1)
    expect_err("net.syns[(0, 1)][1].event(0.1, 1)")
    del net
    cv.active(0)
    h.dt = 0.025
    a = h.IntFire1()
    nc = h.NetCon(None, a)
    h.finitialize()
    nc.weight[0] = 2
    nc.event(0)  # when delivered a fires and turns on refractory
    h.fadvance()
    assert a.m == 2.0
    nc.weight[0] = 0.2
    nc.event(h.t, 1)  # when delivered turns off refractory
    h.fadvance()
    assert a.m == 0
    nc.event(h.t)  # so when delivered this event changes m
    h.fadvance()
    assert a.m == 0.2

    # NetCon.record cover a line, NetCon.get_recordvec, NetCon.wcnt
    assert nc.wcnt() == 1
    tv = h.Vector()
    nc.record(tv)
    assert nc.get_recordvec() == tv
    nc.record()
    nc = h.NetCon(None, None)
    expect_err("nc.record()")  # cover static void NetCon::chksrc()

    # NetCon.srcgid cover a line
    nc = h.NetCon(a, a)
    del a
    assert nc.srcgid() == -1
    del nc

    # NetCon constructor errors and some coverage
    expect_err("h.NetCon([], None)")
    expect_err("h.NetCon(None, [])")
    h.NetCon(None, None, -10.0, 2.0, 0.001)
    a = h.IntFire1()
    h.NetCon(a, a, -10.0, 2.0, 0.001)
    net = Net(2)
    h.NetCon(
        net.cells[0].soma(0.5)._ref_v, None, -10.0, 2.0, 0.001, sec=net.cells[0].soma
    )
    del net, a

    # CVode.netconlist
    net = Net(2, True)  # Hoc Cell test til nrn_sec2cell memory leak fixed
    a = h.IntFire1()
    b = h.NetStim()
    nc = h.NetCon(b, a)
    lst = cv.netconlist("", "", "")
    assert len(h.List("NetCon")) == 5
    assert len(cv.netconlist(b, "", "")) == 1
    assert len(cv.netconlist("NetStim[<0-9>*]", "", "")) == 1
    assert len(cv.netconlist(b, "", a)) == 1
    assert len(cv.netconlist(b, "", "IntFire1")) == 1
    assert len(cv.netconlist(net.cells[0], "", "")) == 2
    assert len(cv.netconlist(net.cells[0].hname(), "", "")) == 2
    assert len(cv.netconlist("", net.cells[0], "")) == 2
    assert len(cv.netconlist("", net.cells[0].hname(), "")) == 2
    assert len(cv.netconlist("", "", net.syns[0, 1][0])) == 1
    assert len(cv.netconlist("", "", net.syns[0, 1][0].hname())) == 1

    expect_err('cv.netconlist("foo<<", "", "")')
    expect_err('cv.netconlist("", "foo<<",  "")')
    expect_err('cv.netconlist("", "", "foo<<")')
    del lst, nc, a, b, net
    locals()


def cvode_meth():
    s = h.Vector()
    ds = h.Vector()
    cv.states(s)
    assert len(s) == 0
    cv.dstates(ds)
    assert len(ds) == 0

    net = Net(2)
    cv.active(1)
    cv.use_local_dt(0)
    h.finitialize(-65)
    cv.solve(1.0)
    cv.states(s)
    sref = h.ref("")
    snames = [(cv.statename(i, sref), sref[0])[1] for i in range(len(s))]
    print(snames)
    chk("cv.statename", snames)
    chk("cv.states", s)
    cv.dstates(ds)
    chk("cv.dstates", ds)
    std = (h.t, s.to_python(), ds.to_python())
    ds.fill(0)
    cv.f(1.0, s, ds)
    print("std ", std)
    print("(t, s, ds)", (h.t, s.to_python(), ds.to_python()))
    assert (h.t, s.to_python(), ds.to_python()) == std
    s.resize(0)
    expect_err("cv.f(1.0, s, ds)")
    pc.nthread(2)
    h.finitialize(-65)
    s = h.Vector(std[1])
    expect_err("cv.f(std[0], s, ds)")
    pc.nthread(1)
    h.finitialize(-65)

    cv.use_local_dt(1)
    h.finitialize(-65)
    cv.solve(1.0)
    expect_err("cv.f(std[0], s, ds)")
    cv.states(s)
    sref = h.ref("")
    snames = [(cv.statename(i, sref), sref[0])[1] for i in range(len(s))]
    print(snames)
    chk("cv.statename lvardt", snames)
    chk("cv.states lvardt", s)
    cv.dstates(ds)
    chk("cv.dstates lvardt", ds)
    cv.use_local_dt(0)


def test_netcvode_cover():
    nrn_use_daspk()
    node()
    netcon_access()
    cvode_meth()


if __name__ == "__main__":
    test_netcvode_cover()
    chk.save()
    pass

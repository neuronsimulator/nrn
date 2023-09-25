from neuron import h
from neuron.expect_hocerr import expect_err
from neuron.tests.utils.checkresult import Chk

import io, math, os, re, sys

dir_path = os.path.dirname(os.path.realpath(__file__))
chk = Chk(os.path.join(dir_path, "test_netcvode.json"))
if hasattr(h, "usetable_hh"):
    h.usetable_hh = 0  # So same whether compiled with CoreNEURON or not.

cv = h.CVode()
pc = h.ParallelContext()

# remove address info from cv.debug_event output
def debug_event_filter(s):
    s = re.sub(r"cvode_0x[0-9abcdef]* ", "cvode_0x... ", s)
    s = re.sub(r"Vector\[\d+\]", "Vector[N]", s)
    return s


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
        ref_t, ref_x = results[arg[0]][arg[1]]
        assert math.isclose(h.t, ref_t, rel_tol=1e-15)
        assert math.isclose(src.x, ref_x, rel_tol=1e-13)

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
    nc = h.NetCon(None, None)
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
    # empty
    s = h.Vector()
    ds = h.Vector()
    cv.states(s)
    assert len(s) == 0
    cv.dstates(ds)
    assert len(ds) == 0
    h.finitialize(-65)
    cv.event(0.5)
    cv.solve(1.0)
    cv.event(1.1)
    cv.solve()

    cv.active(1)
    pc.nthread(2)
    h.finitialize(-65)
    cv.event(0.5)
    cv.solve(1)
    cv.event(1.01)
    cv.solve()
    pc.nthread(1)
    cv.use_local_dt(1)
    assert cv.current_method() == 300.0
    cv.use_local_dt(0)

    net = Net(2)
    cv.active(1)
    cv.use_local_dt(0)
    h.finitialize(-65)
    cv.solve(1.0)
    cv.statistics()
    cv.spike_stat(ds)
    # Note: states, dstates, acor are not the same for lvardt
    cv.states(s)
    sref = h.ref("")
    snames = [(cv.statename(i, sref), sref[0])[1] for i in range(len(s))]
    print(snames)
    chk("cv.statename", snames)
    chk("cv.states", s, tol=1e-8)
    cv.dstates(ds)
    chk("cv.dstates", ds, tol=1e-8)
    vec = h.Vector()
    cv.error_weights(vec)
    chk("cv.error_weights", vec)
    cv.acor(vec)
    chk("cv.acor", vec, tol=1e-7)
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
    cv.statistics()
    cv.spike_stat(ds)
    expect_err("cv.f(std[0], s, ds)")
    cv.states(s)
    sref = h.ref("")
    snames = [(cv.statename(i, sref), sref[0])[1] for i in range(len(s))]
    chk("cv.statename lvardt", snames)
    chk("cv.states lvardt", s, tol=1e-8)
    cv.dstates(ds)
    chk("cv.dstates lvardt", ds, tol=1e-8)
    cv.error_weights(vec)
    chk("cv.error_weights lvardt", vec)
    cv.acor(vec)
    chk("cv.acor lvardt", vec, tol=5e-7)
    h.stoprun = 1
    cv.solve()
    cv.use_local_dt(0)
    expect_err("cv.statename(1e9, sref)")
    cv.active(0)
    expect_err("cv.statename(0, sref)")
    assert vec.size() > 0
    cv.error_weights(vec)
    assert vec.size() == 0
    vec.resize(20)
    cv.acor(vec)
    assert vec.size() == 0
    cv.active(1)

    del net, sref, snames, s, ds, std
    locals()


def state_magnitudes():
    h.load_file("stdrun.hoc")
    h.load_file("atoltool.hoc")
    net = Net(2)
    atool = h.AtolTool()
    h.tstop = 1

    def run():
        xtra = " lvardt" if cv.use_local_dt() else ""
        atool.anrun()
        r = [[i.name, i.max, i.acmax] for i in atool.states]
        chk("AtolTool" + xtra, r, tol=5e-7)
        h.init()
        atool.activate(1)
        h.run()
        cv.state_magnitudes(2)
        vec = h.Vector()
        cv.state_magnitudes(vec, 0)
        # bug for lvardt. slot for second cell is all 0
        chk("state_magnitudes states" + xtra, vec, tol=5e-8)
        cv.state_magnitudes(vec, 1)
        chk("state_magnitudes acor" + xtra, vec, tol=5e-7)
        cv.state_magnitudes(0)

    run()
    cv.use_local_dt(1)
    run()
    cv.use_local_dt(0)


def vec_record_discrete():
    net = Net(2)
    vec = h.Vector()
    trecord = h.Vector()
    tvec = h.Vector().indgen(0.1, 0.8, 0.1)
    vec.record(net.cells[0].soma(0.5)._ref_v, tvec, sec=net.cells[0].soma)
    trecord.record(h._ref_t, tvec, sec=net.cells[0].soma)
    cv.active(1)

    def run(tstop):
        pc.set_maxstep(10)
        h.finitialize(-65)
        h.frecord_init()
        pc.psolve(tstop)

    def ssrun(tstop):
        ss = h.SaveState()
        run(tstop / 2.0)
        ss.save()
        h.finitialize(-65)
        ss.restore()
        pc.psolve(tstop)

    run(1)
    chk("record discrete tvec", vec, tol=1e-9)
    tvec.indgen(1.1, 1.8, 0.1)
    ssrun(2)
    chk("record discrete savestate tvec", vec, tol=5e-7)
    cv.record_remove(vec)
    vec.record(net.cells[0].soma(0.5)._ref_v, 0.1, sec=net.cells[0].soma)
    trecord.record(h._ref_t, 0.1, sec=net.cells[0].soma)
    run(1)
    chk("record discrete dt", vec, tol=5e-9)
    ssrun(2)
    chk("record discrete savestate dt", vec, tol=5e-7)


def integrator_properties():
    s = h.Section()
    s.insert("pas")
    s.e_pas = 0
    s.g_pas = 0.001
    cv.active(1)
    vvec = h.Vector()
    tvec = h.Vector()
    vvec.record(s(0.5)._ref_v, sec=s)
    tvec.record(h._ref_t, sec=s)

    # rtol and atol
    def run1(key):
        h.finitialize(0.001)
        cv.solve(2)
        chk(key + " tvec", tvec, tol=5e-10)
        chk(key + " vvec", vvec, tol=1e-9)

    cv.rtol(1e-3)
    cv.atol(0)
    run1("rtol=%g atol=%g" % (cv.rtol(), cv.atol()))
    cv.rtol(0)
    cv.atol(1e-3)
    run1("rtol=%g atol=%g" % (cv.rtol(), cv.atol()))
    del s

    net = Net(2)
    vvec.record(net.cells[0].soma(0.5)._ref_v, sec=net.cells[0].soma)
    tvec.record(h._ref_t, sec=net.cells[0].soma)

    # stiff
    def run2(key):
        h.finitialize(-65)
        cv.solve(2)
        # Note the very large tolerance, this seems relatively unstable with the
        # NVIDIA compilers
        chk(key + " tvec size", tvec.size(), tol=0.25)

    cv.use_local_dt(1)
    cv.stiff(0)
    run2("stiff=0 lvardt")
    cv.stiff(2)
    cv.use_local_dt(0)
    for stiff in range(3):
        cv.stiff(stiff)
        run2("stiff=%d" % (cv.stiff(),))

    # maxorder, minstep, maxstep
    for lvardt in [1, 0]:
        cv.use_local_dt(lvardt)
        for maxorder in [2, 5]:
            cv.maxorder(maxorder)
            assert cv.maxorder() == maxorder
            h.finitialize(-65)
            while h.t < 2.0:
                cv.solve()
                order = cv.order(0) if lvardt else cv.order()
                assert order <= maxorder
        for minstep in [0.0001, 0.0]:
            cv.minstep(minstep)
            assert cv.minstep() == minstep
        for maxstep in [0.5, 1e9]:
            cv.maxstep(maxstep)
            assert cv.maxstep() == maxstep

    # jacobian
    for jac in [2, 1, 0]:  # end up as default
        cv.jacobian(jac)
        run2("jacobian " + str(cv.jacobian()))

    # selfqueue remove
    cv.active(0)
    cv.queue_mode(True, True)
    pc.set_maxstep(10)
    h.finitialize(-65)
    pc.psolve(2)
    cv.queue_mode(False, False)  # or later cv.store_events will abort(0)


def event_queue():
    # two cells with 1 and 3 connections to third cell
    def mknet():
        cells = {i: h.IntFire2() for i in range(3)}
        tvec = h.Vector()
        idvec = h.Vector()
        for i, cell in cells.items():
            cell.ib = 1.5 - 0.1 * i
            h.NetCon(cell, None).record(tvec, idvec, i)
        ncs = {}
        for i, numnc in enumerate([2, 3]):
            for j in range(numnc):
                nc = h.NetCon(cells[i], cells[2])
                ncs[(i, j)] = nc
                nc.weight[0] = -0.01
                nc.delay = 3
        return (cells, ncs, tvec, idvec)

    net = mknet()
    vecstore = h.Vector()
    cv.store_events(vecstore)
    h.finitialize()
    cv.solve(13)  # two NetCon from fast cell 0 and 1 presyn from slower cell 1
    cv.store_events()
    chk("store_events", vecstore, tol=1e-14)
    chk("event queue spikes", [net[2].to_python(), net[3].to_python()], tol=5e-16)
    old_stdout = sys.stdout
    sys.stdout = mystdout = io.StringIO()
    cv.print_event_queue()
    sys.stdout = old_stdout
    chk("print_event_queue", mystdout.getvalue(), tol=1e-14)
    tvec = h.Vector()
    cv.print_event_queue(tvec)
    chk("print_event_queue tvec", tvec, tol=1e-14)
    objs = h.List()
    flagvec = h.Vector()
    cv.event_queue_info(2, tvec, objs)
    chk(
        "event_queue_info(2...)",
        [tvec.to_python(), [o.hname() for o in objs]],
        tol=5e-16,
    )
    cv.event_queue_info(3, tvec, flagvec, objs)
    chk(
        "event_queue_info(3...)",
        [tvec.to_python(), flagvec.to_python(), [o.hname() for o in objs]],
        tol=5e-16,
    )

    # some savestate coverage
    def run(tstop):
        h.finitialize()
        cv.solve(tstop)

    run(25)

    def raster():
        print("spiketimes")
        net[2].printf()
        print("ids")
        net[3].printf()

    run(13)
    ss = h.SaveState()
    ss.save()
    sf = h.File("ss.bin")
    ss.fwrite(sf)
    del ss
    h.finitialize()
    ss = h.SaveState()
    ss.fread(sf)
    ss.restore()
    cv.solve(25)
    chk("SaveState restore at 13", [net[2].to_python(), net[3].to_python()], tol=5e-16)


def scatter_gather():
    net = Net(1)
    h.finitialize(-65)
    cv.solve(2)
    svec = h.Vector()
    cv.states(svec)
    yvec = h.Vector()
    cv.ygather(yvec)
    assert yvec.eq(svec)
    h.finitialize(-65)
    cv.yscatter(yvec)
    cv.re_init()
    cv.states(svec)
    assert yvec.eq(svec)
    cv.use_local_dt(1)
    expect_err("cv.yscatter(yvec)")
    expect_err("cv.ygather(yvec)")
    cv.use_local_dt(0)
    pc.nthread(2)
    expect_err("cv.yscatter(yvec)")
    expect_err("cv.ygather(yvec)")
    pc.nthread(1)
    yvec.resize(0)
    expect_err("cv.yscatter(yvec)")
    del yvec, svec, net
    locals()


def playrecord():
    net = Net(1)
    net.ic.dur = 1.0
    tvec = h.Vector([0, 1, 1])
    avec = h.Vector([0, 0.1, 0])
    avec.play(net.ic._ref_amp, tvec)
    cv.active(1)
    cv.debug_event(1)
    old_stdout = sys.stdout
    sys.stdout = mystdout = io.StringIO()
    h.finitialize(-65)
    cv.solve(1)
    cv.debug_event(0)
    sys.stdout = old_stdout
    chk("playrecord debug_event", debug_event_filter(mystdout.getvalue()))
    del net.ic
    h.finitialize(-65)


def interthread():
    net = Net(4)
    cv.active(0)
    h.dt = 0.025
    cv.debug_event(1)
    pc.nthread(2, 0)  # serial threads avoid race on print callback
    old_stdout = sys.stdout
    sys.stdout = mystdout = io.StringIO()
    h.finitialize(-65)
    while h.t < 10:
        h.fadvance()
    sys.stdout = old_stdout
    chk("interthread debug_event 2 serial threads", mystdout.getvalue())
    pc.nthread(1)
    cv.debug_event(0)


def nc_event_before_init():
    soma = h.Section()
    soma.insert("pas")

    syn = h.ExpSyn(soma(0.5))
    nc = h.NetCon(None, syn)
    nc.weight[0] = 1.0
    # h.finitialize()
    expect_err("nc.event(0)")  # nrn_assert triggered if outside of finitialize


def test_netcvode_cover():
    nrn_use_daspk()
    node()
    netcon_access()
    cvode_meth()
    state_magnitudes()
    vec_record_discrete()
    integrator_properties()
    event_queue()
    scatter_gather()
    playrecord()
    interthread()
    nc_event_before_init()


if __name__ == "__main__":
    test_netcvode_cover()
    chk.save()
    pass

# Test proper management of HOC PythonObject (no memory leaks)
# Expanded for more testing and coverage of BBSaveState.
from glob import iglob
from neuron import h
from neuron.units import ms, mV

import numpy as np
import os
import shutil
import subprocess


def hlist():
    return h.List("PythonObject")


def test_1():
    hl = hlist()
    npo = hl.count()  # compare against this. Previous tests may leave some.

    h("objref po")
    h.po = {}
    assert hl.count() == (npo + 1)
    h.po = h.Vector()
    assert hl.count() == npo
    h.po = None
    assert hl.count() == npo

    h("po = new PythonObject()")
    assert hl.count() == (npo + 1)
    h("po = po.list()")
    assert type(h.po) == type([])
    assert hl.count() == (npo + 1)
    h("objref po")
    assert hl.count() == npo

    h(
        """obfunc test_hoc_po() { localobj foo
         foo = new PythonObject()
         return foo.list()
       }
       po = test_hoc_po()
    """
    )
    assert hl.count() == (npo + 1)
    h.po = None
    assert hl.count() == npo
    assert type(h.test_hoc_po()) == type([])
    assert hl.count() == npo

    # nrn.Section tests
    for sec in h.allsec():
        h.delete_section(sec=sec)

    h("""create soma""")
    h.po = h.soma
    assert h.po == h.soma
    assert hl.count() == (npo + 1)
    h.po = None
    assert hl.count() == npo
    h.po = h.soma
    h.delete_section(sec=h.soma)
    assert str(h.po) == "<deleted section>"
    h.po = None
    assert hl.count() == npo

    h.po = h.Section(name="pysoma")
    h("po.L = 5")
    assert h.po.L == 5
    assert hl.count() == (npo + 1)
    h.po = None
    assert hl.count() == npo
    assert len([sec for sec in h.allsec()]) == 0


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


def test_2():
    hl = hlist()
    npo = hl.count()

    r = Ring(3, 2)
    assert hl.count() == npo
    for gid in r.cells:
        assert pc.gid2cell(gid) == r.cells[gid]
    assert hl.count() == npo
    pc.gid_clear()


# BBSaveState for mixed (hoc and python cells) Ring.

# some helpers copied from ../parallel_tests/test_bas.py
def rmfiles():
    if pc.id() == 0:
        shutil.rmtree("bbss_out", ignore_errors=True)
        shutil.rmtree("in", ignore_errors=True)
        shutil.rmtree("binbufout", ignore_errors=True)
        shutil.rmtree("binbufin", ignore_errors=True)
        os.mkdir("binbufout")
        try:
            os.unlink("allcell-bbss.dat")
        except FileNotFoundError:
            pass
    pc.barrier()


def cp_out_to_in():
    if pc.id() == 0:
        shutil.rmtree("in", ignore_errors=True)
        os.mkdir("in")
        shutil.copyfile("bbss_out/tmp", "in/tmp")
        for f in iglob("bbss_out/tmp.*.*"):
            # Get A from bbss_out/tmp.A.B
            i = os.path.basename(f).split(".", 2)[1]
            if not os.path.isfile("in/tmp.{}".format(i)):
                files = list(iglob("bbss_out/tmp.{}.*".format(i)))
                cnt = len(files)
                with open("in/tmp.{}".format(i), "w") as ofile:
                    ofile.write("{}\n".format(cnt))
                    for fname in files:
                        with open(fname, "r") as ifile:
                            shutil.copyfileobj(ifile, ofile)
    pc.barrier()


def prun(tstop, mode=None):
    pc.set_maxstep(10 * ms)
    h.finitialize(-65 * mV)

    if mode == "save_test":  # integrate to tstop/2 and save
        pc.psolve(tstop / 2)
        bbss = h.BBSaveState()
        bbss.save_test()  # creates separate file in bbss_out for each piece
    elif mode == "save_test_bin":
        pc.psolve(tstop / 2)
        bbss = h.BBSaveState()
        bbss.save_test_bin()  # like save_test but binary wholecell in binbufout
    elif mode == "save":
        pc.psolve(tstop / 2)
        bbss = h.BBSaveState()
        bbss.save("allcell-bbss.dat")  # single file for everything (nhost==1!)
    elif mode == "restore_test":
        cp_out_to_in()  # prepare for restore.
        bbss = h.BBSaveState()
        bbss.restore_test()
    elif mode == "restore_test_bin":
        shutil.copytree("binbufout", "binbufin")
        bbss = h.BBSaveState()
        bbss.restore_test_bin()
    elif mode == "restore":
        bbss = h.BBSaveState()
        bbss.restore("allcell-bbss.dat")
        bbss.vector_play_init()  # just for coverage as there are none.

    pc.psolve(tstop)  # integrate the rest of the way


def get_all_spikes(ring):
    local_data = {}
    for i, gid in enumerate(ring.spikegid):
        if gid not in local_data:
            local_data[gid] = []
        local_data[gid].append(ring.spiketime[i])
    all_data = pc.py_allgather([local_data])

    pc.barrier()

    data = {}
    for d in all_data:
        data.update(d[0])

    return data


def compare_dicts(dict1, dict2):
    # assume dict is {gid:[spiketime]}

    # In case iteration order not same in dict1 and dict2, use dict1 key
    # order to access dict
    keylist = dict1.keys()

    # verify same set of keys
    assert set(keylist) == set(dict2.keys())

    # verify same count of spikes for each key
    assert [len(dict1[k]) for k in keylist] == [len(dict2[k]) for k in keylist]

    # Put spike times in array so can compare with a tolerance.
    array_1 = np.array([val for k in keylist for val in dict1[k]])
    array_2 = np.array([val for k in keylist for val in dict2[k]])
    assert np.allclose(array_1, array_2)


def test_3():
    rmfiles()
    hl = hlist()
    npo = hl.count()

    stdspikes = {
        0.0: [
            1.975000000099995,
            21.300000000099324,
            39.70000000010047,
            58.05000000010464,
            76.4000000001088,
            94.80000000011299,
        ],
        1.0: [
            5.9250000001000505,
            24.800000000099125,
            43.32500000010129,
            61.725000000105474,
            80.10000000010965,
            98.47500000011382,
        ],
        2.0: [
            9.900000000099972,
            28.40000000009892,
            46.950000000102115,
            65.3750000001063,
            83.77500000011048,
        ],
        3.0: [
            13.900000000099745,
            32.15000000009875,
            50.600000000102945,
            69.02500000010713,
            87.45000000011132,
        ],
        4.0: [
            17.900000000099517,
            36.02500000009963,
            54.32500000010379,
            72.70000000010796,
            91.12500000011215,
        ],
    }

    ring = Ring(3, 2)
    assert hl.count() == npo

    prun(100 * ms)
    if "usetable_hh" not in dir(h):  # coreneuron has different hh.mod
        stdspikes = get_all_spikes(ring)
    compare_dicts(get_all_spikes(ring), stdspikes)
    stdspikes_after_50 = {}
    for gid in stdspikes:
        stdspikes_after_50[gid] = [spk_t for spk_t in stdspikes[gid] if spk_t >= 50.0]

    prun(100 * ms, mode="save_test")  # at tstop/2 does a BBSaveState.save
    assert hl.count() == npo
    compare_dicts(get_all_spikes(ring), stdspikes)

    prun(100 * ms, mode="restore_test")  # BBSaveState restore to start at t = tstop/2
    assert hl.count() == npo
    compare_dicts(get_all_spikes(ring), stdspikes_after_50)

    # while we are at it check the BBSaveState binary file mode.
    prun(100 * ms, mode="save_test_bin")
    compare_dicts(get_all_spikes(ring), stdspikes)
    prun(100 * ms, mode="restore_test_bin")
    compare_dicts(get_all_spikes(ring), stdspikes_after_50)

    # while we are at it check the BBSaveState save/restore single file mode.
    prun(100 * ms, mode="save")
    compare_dicts(get_all_spikes(ring), stdspikes)
    prun(100 * ms, mode="restore")
    compare_dicts(get_all_spikes(ring), stdspikes_after_50)
    assert hl.count() == npo

    # save_request works but not very useful as save_gid and restore_gid
    # not implemented.
    pc.set_maxstep(10)
    h.finitialize(-65)
    gids = h.Vector()
    sizes = h.Vector()
    bbss = h.BBSaveState()
    ngid = bbss.save_request(gids, sizes)
    assert ngid == len(ring.cells)
    # prints "not implemented"
    bbss.save_gid()
    bbss.restore_gid()
    assert hl.count() == npo
    del bbss

    # binq
    rmfiles()
    h.CVode().queue_mode(1)
    prun(100, mode="save")
    compare_dicts(get_all_spikes(ring), stdspikes)
    prun(100, mode="restore")
    compare_dicts(get_all_spikes(ring), stdspikes_after_50)
    prun(100, mode="save_test_bin")
    compare_dicts(get_all_spikes(ring), stdspikes)
    prun(100, mode="restore_test_bin")
    compare_dicts(get_all_spikes(ring), stdspikes_after_50)
    h.CVode().queue_mode(0)

    pc.gid_clear()


def run2(tstop, mode=None):
    pc.set_maxstep(10)
    h.finitialize()
    if mode == "save_test":
        pc.psolve(tstop / 2)
        h.BBSaveState().save_test()
    elif mode == "restore_test":
        cp_out_to_in()
        h.BBSaveState().restore_test()
    pc.psolve(tstop)


def cmpspikes2(tbegin, tvec, gidvec, tvec_std, gidvec_std):
    i = tvec.indwhere(">=", tbegin)
    j = tvec_std.indwhere(">=", tbegin)
    assert tvec.c(i).eq(tvec_std.c(j))
    assert gidvec.c(i).eq(gidvec_std.c(j))


def test_4():  # for some extra coverage of BBSaveState
    # Save and restore SelfEvent, Random123, NetStims hava a gid
    # NetStim ring
    ncell = 2
    gids = range(pc.id(), ncell, pc.nhost())
    cells = {}
    for gid in gids:
        cells[gid] = h.NetStim()
        cell = cells[gid]
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(cell, None))
        cell.interval = 1
        cell.number = 3  # >1 has no effect but helps with coverage
        cell.start = 1 if gid == 0 else -1  # gid 0 initiates spike
        cell.noise = 0.1
        cell.noiseFromRandom123(gid, 0, 0)
    netcons = [pc.gid_connect((gid + ncell - 1) % ncell, cells[gid]) for gid in gids]
    for netcon in netcons:
        netcon.weight[0] = 1

    spiketimes = h.Vector()
    spikegids = h.Vector()
    pc.spike_record(-1, spiketimes, spikegids)

    tstop = 20
    run2(tstop)
    spiketimes_std = spiketimes.c()
    spikegids_std = spikegids.c()

    run2(tstop, mode="save_test")
    cmpspikes2(0.0, spiketimes, spikegids, spiketimes_std, spikegids_std)

    run2(tstop, mode="restore_test")
    cmpspikes2(tstop / 2.0, spiketimes, spikegids, spiketimes_std, spikegids_std)

    pc.gid_clear()


def test_5():
    # NetStim (with no gid) stimulates intrinsically firing IntFire2
    # which stimulates a soma with ExpSyn
    gids = range(pc.id(), 1, pc.nhost())
    cells = {}
    for gid in gids:
        cells[gid] = h.IntFire2()
        cell = cells[gid]
        cell.ib = 2
        cell.taus = 2
        cell.taum = 1

        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(cell, None))

    if pc.gid_exists(0):
        ns = h.NetStim()
        nc = h.NetCon(ns, cells[0])
        ns.interval = 1
        ns.start = 0.2
        nc.weight[0] = 0.01

    spiketimes = h.Vector()
    spikegids = h.Vector()
    pc.spike_record(-1, spiketimes, spikegids)

    tstop = 10
    run2(tstop)
    spiketimes_std = spiketimes.c()
    spikegids_std = spikegids.c()

    run2(tstop, mode="save_test")
    cmpspikes2(0.0, spiketimes, spikegids, spiketimes_std, spikegids_std)

    run2(tstop, mode="restore_test")
    cmpspikes2(tstop / 2.0, spiketimes, spikegids, spiketimes_std, spikegids_std)
    for i, t in enumerate(spiketimes):
        print(t, spikegids[i])

    pc.gid_clear()


def test_6():  # Test BBSaveState.ignore
    rmfiles()
    model = Ring(5, 0)
    syn = model.cells[0].syn
    prun(1)
    h.finitialize(-65)
    g_expect = syn.g
    pc.psolve(20)
    g_actual = syn.g
    bbss = h.BBSaveState()
    bbss.ignore(syn)
    bbss.save("allcell-bbss.dat")
    h.finitialize(-65)
    bbss.restore("allcell-bbss.dat")
    print(g_expect, g_actual, syn.g)
    assert syn.g == g_expect
    h.finitialize(-65)
    pc.psolve(20)
    bbss.ignore(syn)
    bbss.ignore()
    bbss.save("allcell-bbss.dat")
    h.finitialize(-65)
    bbss.restore("allcell-bbss.dat")
    print(g_expect, g_actual, syn.g)
    assert abs((syn.g - g_actual) / (syn.g + g_actual)) < 1e-12

    pc.gid_clear()


def test_7():  # save a ring with n cells and extra section, restore n-1 cells
    rmfiles()
    model = Ring(5, 0)
    model.cells[0].foo = h.Section(name="foo", cell=model.cells[0])
    prun(5)
    v_expect = model.cells[0].soma(0.5).v
    bbss = h.BBSaveState()
    bbss.save_test()
    model = Ring(4, 0)
    prun(0.1)
    bbss = h.BBSaveState()
    cp_out_to_in()
    bbss.restore_test()
    print(v_expect, model.cells[0].soma(0.5).v)
    assert abs(v_expect - model.cells[0].soma(0.5).v) < 1e-13

    pc.gid_clear()


def test_8():
    print("focus on BinQ initialization Issue #1444")
    ncell = 3
    n = 5
    # gid -> [n NetCons] -> gid+1
    cells = {gid: h.Follower() for gid in range(pc.id(), ncell, pc.nhost())}
    for gid in cells:
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(cells[gid], None))
    netcons = {}
    for gid in cells:
        netcons[gid] = [
            pc.gid_connect(gid - 1, cells[gid]) for _ in range(n) if gid > 0
        ]
    # Stimulate gid 0
    if 0 in cells:
        # stim cell[0] with fast burst of 5 spikes
        ns = h.NetStim()
        ns.start = 0.9999
        ns.number = 5
        ns.interval = 0.1
        nsnc = h.NetCon(ns, cells[0])
        nsnc.delay = 0
        nsnc.weight[0] = 0
    for gid, cell in cells.items():
        # cells generate spike on every input of weight > 1
        # cell.refrac = 0
        for i, nc in enumerate(netcons[gid]):
            nc.weight[0] = 0.001 * i + h.dt
            nc.delay = (1.0 + 0.01 * i) * h.dt
    spiketime = h.Vector()
    spikegid = h.Vector()
    pc.spike_record(-1, spiketime, spikegid)

    def run(tstop):
        pc.set_maxstep(10)
        h.finitialize()
        pc.psolve(tstop)

    spiketime_std = spiketime.c()
    spikegid_std = spikegid.c()

    def set_stdspikes():
        spiketime_std.copy(spiketime)
        spikegid_std.copy(spikegid)

    tstop = 5
    run(tstop)
    set_stdspikes()

    def prspikes():
        print("prspikes")
        for i, gid in enumerate(spikegid):
            print("%g %d" % (spiketime[i], gid))

    def compare_spikes():
        x = list(zip(spiketime_std, spikegid_std))
        x = sorted(x, key=lambda e: e[1])
        y = list(zip(spiketime, spikegid))
        y = sorted(y, key=lambda e: e[1])
        if len(x) != len(y):
            print(len(x), len(y))
        # assert len(x) == len(y)
        if x != y:
            q = (x, y) if len(x) <= len(y) else (y, x)
            for i, a in enumerate(q[0]):
                b = q[1][i]
                if a != b:
                    # assert a[1] == b[1]
                    z = abs(a[0] - b[0])
                    if z >= 1e-9:
                        print(x[i], y[i])
                    assert z < 1e-9

    def srun(tsave, tstop):
        cvode = h.CVode()
        qm = cvode.queue_mode()
        print(
            "srun tsave=%g tstop=%g start=%g delay=%g %s"
            % (tsave, tstop, ns.start, nsnc.delay, "binq" if qm else "")
        )
        run(tsave)
        st = spiketime.c()
        sg = spikegid.c()
        bbss = h.BBSaveState()
        bbss.save("allcell_bbss.dat")
        qm = 0
        if qm:
            print("after save")
            h.CVode().print_event_queue()
        h.finitialize(-65)
        spiketime.resize(0)
        spikegid.resize(0)
        bbss.restore("allcell_bbss.dat")
        if qm:
            print("after restore")
            h.CVode().print_event_queue()
        pc.psolve(tstop)
        st.append(spiketime)
        sg.append(spikegid)
        spiketime.copy(st)
        spikegid.copy(sg)
        print("    %d spikes" % len(spiketime))

    srun(1.1, tstop)
    compare_spikes()

    cvode = h.CVode()
    cvode.queue_mode(1)
    run(tstop)
    assert len(spiketime_std) == len(spiketime)
    spiketime_std = spiketime.c()
    spikegid_std = spikegid.c()

    srun(1.1, tstop)
    compare_spikes()
    for binq in [0, 1]:
        cvode.queue_mode(binq)
        parms = [
            (1, 0),
            (1e-10, 0),
            (0, 0),
            (0.25 * h.dt, 0),
            (0.5 * h.dt, 0),
            (0.75 * h.dt, 0),
        ]
        for parm in parms:
            if 0 in cells:
                ns.start, nsnc.delay = parm
            for tsave in [0.0, h.dt]:
                run(tstop)
                set_stdspikes()
                srun(tsave, tstop)
                compare_spikes()

    cvode.queue_mode(0)
    pc.gid_clear()


if __name__ == "__main__":
    test_1()
    test_2()
    test_3()
    test_4()
    test_5()
    test_6()
    test_7()
    test_8()

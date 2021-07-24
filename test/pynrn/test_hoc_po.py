# Test proper management of HOC PythonObject (no memory leaks)

from neuron import h
from neuron.units import ms, mV

import numpy as np
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
def subprocess_run(cmd):
  subprocess.run(cmd, shell=True).check_returncode()

def rmfiles():
    if pc.id() == 0:
        subprocess_run("rm -r -f bbss_out")
        subprocess_run("rm -r -f in")
        subprocess_run("rm -r -f binbufout")
        subprocess_run("rm -r -f binbufin")
        subprocess_run("mkdir binbufout")
        subprocess_run("rm -f allcell-bbss.dat")
    pc.barrier()


def cp_out_to_in():
    out2in_sh = r"""
#!/bin/bash
out=bbss_out
rm -f in/*
mkdir in
cat $out/tmp > in/tmp
for f in $out/tmp.*.* ; do
  i=`echo "$f" | sed 's/.*tmp\.\([0-9]*\)\..*/\1/'`
  if test ! -f in/tmp.$i ; then
    cnt=`ls $out/tmp.$i.* | wc -l`
    echo $cnt > in/tmp.$i
    cat $out/tmp.$i.* >> in/tmp.$i
  fi
done    
"""
    if pc.id() == 0:
        import tempfile

        with tempfile.NamedTemporaryFile("w") as scriptfile:
            scriptfile.write(out2in_sh)
            scriptfile.flush()
            subprocess.check_call(["/bin/bash", scriptfile.name])

    pc.barrier()


def prun(tstop, mode=None):
    pc.set_maxstep(10 * ms)
    h.finitialize(-65 * mV)

    if mode == "save_test":  # integrate to tstop/2 and save
        pc.psolve(tstop / 2)
        bbss = h.BBSaveState()
        bbss.save_test() # creates separate file in bbss_out for each piece
    elif mode == "save_test_bin":
        pc.psolve(tstop / 2)
        bbss = h.BBSaveState()
        bbss.save_test_bin() # like save_test but binary wholecell in binbufout
    elif mode == "save":
        pc.psolve(tstop / 2)
        bbss = h.BBSaveState()
        bbss.save("allcell-bbss.dat") # single file for everything (nhost==1!)
    elif mode == "restore_test":
        cp_out_to_in()  # prepare for restore.
        bbss = h.BBSaveState()
        bbss.restore_test()
    elif mode == "restore_test_bin":
        subprocess_run("mkdir binbufin")
        subprocess_run("cp binbufout/* binbufin")
        bbss = h.BBSaveState()
        bbss.restore_test_bin()
    elif mode == "restore": 
        bbss = h.BBSaveState()
        bbss.restore("allcell-bbss.dat")

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

    pc.gid_clear()


if __name__ == "__main__":
    test_1()
    test_2()
    test_3()
    h.allobjects()

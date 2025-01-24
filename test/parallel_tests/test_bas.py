from neuron import h
from neuron.units import ms, mV

h("objref po")
h.po = {}

import numpy as np
import subprocess

pc = h.ParallelContext()


# start fresh with respect to SaveState and BBSaveState
def rmfiles():
    if pc.id() == 0:
        subprocess.run("rm -f state*.bin", shell=True)
        subprocess.run("rm -r -f bbss_out", shell=True)
        subprocess.run("rm -r -f in", shell=True)
    pc.barrier()


rmfiles()


class Cell:
    def __init__(self, gid, x, y, z, theta):
        self._gid = gid
        self._setup_morphology()
        self.all = self.soma.wholetree()
        self._setup_biophysics()
        self.x = self.y = self.z = 0
        h.define_shape()
        self._rotate_z(theta)
        self._set_position(x, y, z)

        self._spike_detector = h.NetCon(self.soma(0.5)._ref_v, None, sec=self.soma)
        self.spike_times = h.Vector()
        self._spike_detector.record(self.spike_times)

        self._ncs = []

        self.soma_v = h.Vector().record(self.soma(0.5)._ref_v)

    def __repr__(self):
        return "{}[{}]".format(self.name, self._gid)

    def _set_position(self, x, y, z):
        for sec in self.all:
            for i in range(sec.n3d()):
                sec.pt3dchange(
                    i,
                    x - self.x + sec.x3d(i),
                    y - self.y + sec.y3d(i),
                    z - self.z + sec.z3d(i),
                    sec.diam3d(i),
                )
        self.x, self.y, self.z = x, y, z

    def _rotate_z(self, theta):
        """Rotate the cell about the Z axis."""
        for sec in self.all:
            for i in range(sec.n3d()):
                x = sec.x3d(i)
                y = sec.y3d(i)
                c = h.cos(theta)
                s = h.sin(theta)
                xprime = x * c - y * s
                yprime = x * s + y * c
                sec.pt3dchange(i, xprime, yprime, sec.z3d(i), sec.diam3d(i))


class BallAndStick(Cell):
    name = "BallAndStick"

    def _setup_morphology(self):
        self.soma = h.Section(name="soma", cell=self)
        self.dend = h.Section(name="dend", cell=self)
        self.dend.connect(self.soma)
        self.soma.L = self.soma.diam = 12.6157
        self.dend.L = 200
        self.dend.diam = 1

    def _setup_biophysics(self):
        for sec in self.all:
            sec.Ra = 100  # Axial resistance in Ohm * cm
            sec.cm = 1  # Membrane capacitance in micro Farads / cm^2
        self.soma.insert("hh")
        for seg in self.soma:
            seg.hh.gnabar = 0.12  # Sodium conductance in S/cm2
            seg.hh.gkbar = 0.036  # Potassium conductance in S/cm2
            seg.hh.gl = 0.0003  # Leak conductance in S/cm2
            seg.hh.el = -54.3  # Reversal potential in mV
        # Insert passive current in the dendrite
        self.dend.insert("pas")
        for seg in self.dend:
            seg.pas.g = 0.001  # Passive conductance in S/cm2
            seg.pas.e = -65  # Leak reversal potential mV

        self.syn = h.ExpSyn(self.dend(0.5))
        self.syn.tau = 2 * ms


class Ring:
    """A network of *N* ball-and-stick cells where cell n makes an
    excitatory synapse onto cell n + 1 and the last, Nth cell in the
    network projects to the first cell.
    """

    def __init__(
        self,
        N=5,
        stim_w=0.04,
        stim_t=9,
        stim_delay=1,
        syn_w=0.01,
        syn_delay=25,
        r=50,
    ):
        """
        :param N: Number of cells.
        :param stim_w: Weight of the stimulus
        :param stim_t: time of the stimulus (in ms)
        :param stim_delay: delay of the stimulus (in ms)
        :param syn_w: Synaptic weight
        :param syn_delay: Delay of the synapse
        :param r: radius of the network
        """
        self._N = N
        self.set_gids()  ### assign gids to processors
        self._syn_w = syn_w
        self._syn_delay = syn_delay
        self._create_cells(r)
        self._connect_cells()
        ### stimulate gid 0
        if pc.gid_exists(0):
            self._netstim = h.NetStim()
            self._netstim.number = 1
            self._netstim.start = stim_t
            self._nc = h.NetCon(
                self._netstim, pc.gid2cell(pc.id()).syn
            )  ### grab cell with gid==0 wherever it exists
            self._nc.delay = stim_delay
            self._nc.weight[0] = stim_w

    def set_gids(self):
        """Set the gidlist on this host."""
        #### Round-robin counting.
        #### Each host has an id from 0 to pc.nhost() - 1.
        self.gidlist = list(range(pc.id(), self._N, pc.nhost()))
        for gid in self.gidlist:
            pc.set_gid2node(gid, pc.id())

    def _create_cells(self, r):
        self.cells = {}
        for i in self.gidlist:  ### only create the cells that exist on this host
            theta = i * 2 * h.PI / self._N
            self.cells[i] = BallAndStick(
                i, h.cos(theta) * r, h.sin(theta) * r, 0, theta
            )

        ### associate the cell with this host and gid
        for cell in self.cells.values():
            pc.cell(cell._gid, cell._spike_detector)

    def _connect_cells(self):
        for target in self.cells.values():
            source_gid = (target._gid - 1 + self._N) % self._N
            nc = pc.gid_connect(source_gid, target.syn)
            nc.weight[0] = self._syn_w
            nc.delay = self._syn_delay
            target._ncs.append(nc)


class StarNet:
    """NetStim -> Cell -> N Cells -> Cell"""

    def __init__(self, n):
        self.ncell = [1, n, 1]
        self.nlayer = len(self.ncell)
        self.cells = {}
        # make cells
        for ilayer in range(self.nlayer):
            for icell in range(self.ncell[ilayer]):
                gid = self.info2gid(ilayer, icell)
                if (gid % pc.nhost()) == pc.id():
                    cell = BallAndStick(gid, float(icell), float(ilayer), 0.0, 0.0)
                    self.cells[gid] = cell
                    cell.ilayer = ilayer
                    cell.icell = icell
                    pc.set_gid2node(gid, pc.id())
                    pc.cell(gid, self.cells[gid]._spike_detector)
        # make connections (all to all from layer i-1 to layer i)
        self.nclist = {}
        for gid, cell in self.cells.items():
            if cell.ilayer > 0:
                src_ilayer = cell.ilayer - 1
                for src_icell in range(self.ncell[src_ilayer]):
                    srcgid = self.info2gid(src_ilayer, src_icell)
                    nc = pc.gid_connect(srcgid, cell.syn)
                    nc.weight[0] = 0.01
                    nc.delay = 20
                    self.nclist[(srcgid, gid)] = nc
        # stimulate gid 0 with NetStim
        if 0 in self.cells:
            self.ns = h.NetStim()
            self.ncstim = h.NetCon(self.ns, self.cells[0].syn)
            ns = self.ns
            ns.start = 6
            ns.interval = 10
            ns.number = 100
            nc = self.ncstim
            nc.delay = 2
            nc.weight[0] = 0.01
        # For some extra coverage of BBSaveState::node01
        if 100 in self.cells:
            cell = self.cells[100]
            self.xsyns = [h.ExpSyn(seg) for seg in cell.dend.allseg()]
            self.xnc = []
            for syn in self.xsyns:
                nc = pc.gid_connect(0, syn)
                nc.delay = 20
                nc.weight[0] = 0.0001
                self.xnc.append(nc)

    def info2gid(self, ilayer, icell):
        return ilayer * 100 + icell

    def gid2info(self, gid):
        return (int(gid / 100)), gid % 100

    def topol(self):
        print(pc.id(), self.cells)
        print(pc.id(), self.nclist)


out2in_sh = r"""
#!/usr/bin/env bash
out=bbss_out
rm -f in/*
mkdir -p in
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


def cp_out_to_in():
    if pc.id() == 0:
        import tempfile

        with tempfile.NamedTemporaryFile("w") as scriptfile:
            scriptfile.write(out2in_sh)
            scriptfile.flush()
            subprocess.check_call(["/bin/bash", scriptfile.name])

    pc.barrier()


def prun(tstop, restore=False):
    pc.set_maxstep(10 * ms)
    h.finitialize(-65 * mV)

    if restore == "SaveState":
        ns = h.SaveState()
        sf = h.File("state%d.bin" % pc.id())
        ns.fread(sf)
        ns.restore(0)  # event queue restored
        sf.close()
    elif restore == "BBSaveState":
        cp_out_to_in()  # prepare for restore.
        bbss = h.BBSaveState()
        bbss.restore_test()
    else:
        pc.psolve(tstop / 2)

        # SaveState save
        ss = h.SaveState()
        ss.save()
        sf = h.File("state%d.bin" % pc.id())
        ss.fwrite(sf)
        sf.close()

        # BBSaveState Save
        cnt = h.List("PythonObject").count()
        for i in range(1):
            bbss = h.BBSaveState()
            bbss.save_test()
            bbss = None
        assert h.List("PythonObject").count() == cnt

    pc.psolve(tstop)


def get_all_spikes(ring):
    local_data = {cell._gid: list(cell.spike_times) for cell in ring.cells.values()}
    all_data = pc.py_allgather([local_data])

    pc.barrier()
    pc.done()

    data = {}
    for d in all_data:
        data.update(d[0])

    return data


def compare_dicts(dict1, dict2):
    # assume dict is {gid:[spiketimes]}

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
    if not np.allclose(array_1, array_2):
        print(array_1)
        print(array_2)
        print(array_1 - array_2)
    assert np.allclose(array_1, array_2)


def test_bas():
    # h.execute1(...) does not call mpi_abort on failure
    assert h.execute1("1/0") == 0
    assert h.execute1("2/0", 0) == 0  # no error message printed

    # MPI_Abort can be avoided on hoc errors.
    oldflag = pc.mpiabort_on_error(0)
    assert h("""3/0""") == 0
    try:
        x = h.log(-1)
        assert False
    except:
        assert True
    pc.mpiabort_on_error(oldflag)

    stdspikes = {
        0: [10.925000000099914, 143.3000000001066],
        1: [37.40000000009994, 169.7750000000825],
        2: [63.87500000010596, 196.25000000005844],
        3: [90.35000000011198],
        4: [116.825000000118],
    }

    stdspikes_after_100 = {}
    for gid in stdspikes:
        stdspikes_after_100[gid] = [spk_t for spk_t in stdspikes[gid] if spk_t >= 100.0]

    ring = Ring()

    prun(200 * ms)  # at tstop/2 does a SaveState.save and BBSaveState.save
    stdspikes = get_all_spikes(ring)
    stdspikes_after_100 = {}
    for gid in stdspikes:
        stdspikes_after_100[gid] = [spk_t for spk_t in stdspikes[gid] if spk_t >= 100.0]
    compare_dicts(get_all_spikes(ring), stdspikes)

    prun(200 * ms, "SaveState")  # SaveState restore to start at t = tstop/2
    compare_dicts(get_all_spikes(ring), stdspikes_after_100)

    prun(200 * ms, "BBSaveState")  # BBSaveState restore to start at t = tstop/2
    compare_dicts(get_all_spikes(ring), stdspikes_after_100)


def test_starnet():
    pc.gid_clear()
    starnet = StarNet(8)
    starnet.topol()
    tstop = 100.0
    prun(tstop)
    stdspikes = get_all_spikes(starnet)
    stdspikes_half = {}
    for gid in stdspikes:
        stdspikes_half[gid] = [spk_t for spk_t in stdspikes[gid] if spk_t >= tstop / 2]
    prun(tstop, "BBSaveState")  # BBSaveState restore to start at t = tstop/2
    compare_dicts(get_all_spikes(starnet), stdspikes_half)

    # test for binq mode.
    h.CVode().queue_mode(1)
    prun(tstop)
    compare_dicts(get_all_spikes(starnet), stdspikes)
    prun(tstop, "BBSaveState")
    compare_dicts(get_all_spikes(starnet), stdspikes_half)

    h.dt = 1.0 / 64.0  # issue 1480
    prun(tstop)
    stdspikes = get_all_spikes(starnet)
    stdspikes_half = {}
    for gid in stdspikes:
        stdspikes_half[gid] = [spk_t for spk_t in stdspikes[gid] if spk_t >= tstop / 2]

    prun(tstop, "BBSaveState")
    compare_dicts(get_all_spikes(starnet), stdspikes_half)
    h.CVode().queue_mode(0)
    h.dt = 0.025


if __name__ == "__main__":
    test_bas()
    test_starnet()
    pc.barrier()
    h.quit()

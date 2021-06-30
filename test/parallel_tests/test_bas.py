from neuron import h
from neuron.units import ms, mV

import numpy as np

pc = h.ParallelContext()


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
        ### have one netstim on each rank
        if pc.gid_exists(pc.id()):
            self._netstim = h.NetStim()
            self._netstim.number = 1
            self._netstim.start = stim_t
            self._nc = h.NetCon(
                self._netstim, pc.gid2cell(pc.id()).syn
            )  ### grab cell with gid==pc.id() wherever it exists
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
        self.cells = []
        for i in self.gidlist:  ### only create the cells that exist on this host
            theta = i * 2 * h.PI / self._N
            self.cells.append(
                BallAndStick(i, h.cos(theta) * r, h.sin(theta) * r, 0, theta)
            )
        ### associate the cell with this host and gid
        for cell in self.cells:
            pc.cell(cell._gid, cell._spike_detector)

    def _connect_cells(self):
        for target in self.cells:
            source_gid = (target._gid - 1 + self._N) % self._N
            nc = pc.gid_connect(source_gid, target.syn)
            nc.weight[0] = self._syn_w
            nc.delay = self._syn_delay
            target._ncs.append(nc)


def prun(tstop, restore=False):
    pc.set_maxstep(10 * ms)

    if restore:
        assert np.allclose(h.t, tstop / 2)
        ns = h.SaveState()
        sf = h.File("state%d.bin" % pc.id())
        ns.fread(sf)
        ns.restore(1)
        sf.close()
    else:
        assert np.allclose(h.t, 0)
        pc.psolve(tstop / 2)
        ss = h.SaveState()
        ss.save()

        sf = h.File("state%d.bin" % pc.id())
        ss.fwrite(sf)
        sf.close()
    pc.psolve(tstop)


def get_all_spikes(ring):
    local_data = {cell._gid: list(cell.spike_times) for cell in ring.cells}
    all_data = pc.py_allgather([local_data])

    pc.barrier()
    pc.done()

    data = {}
    for d in all_data:
        data.update(d[0])

    return data


def compare_dicts(dict1, dict2):
    keylist = dict1.keys()
    array_1 = np.array([dict1[key] for key in keylist])
    array_2 = np.array([dict2[key] for key in keylist])

    assert np.allclose(array_1, array_2)


def test_bas():
    ring = Ring()

    h.finitialize(-65 * mV)
    prun(50 * ms)
    compare_dicts(
        get_all_spikes(ring),
        {
            0: [10.925000000099914],
            2: [37.40000000009994],
            4: [],
            1: [10.925000000099914, 37.42500000009995],
            3: [],
        },
    )
    prun(100 * ms, True)
    compare_dicts(
        get_all_spikes(ring),
        {
            0: [10.925000000099914],
            2: [37.40000000009994, 63.90000000010597],
            4: [],
            1: [10.925000000099914, 37.42500000009995],
            3: [63.87500000010596],
        },
    )


if __name__ == "__main__":
    test_bas()
    pc.barrier()
    h.quit()

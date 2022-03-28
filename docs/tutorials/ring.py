from neuron import h
from ballandstick import BallAndStick

### MPI must be initialized before we create a ParallelContext object
h.nrnmpi_init()
pc = h.ParallelContext()


class Ring:
    """A network of *N* ball-and-stick cells where cell n makes an
    excitatory synapse onto cell n + 1 and the last, Nth cell in the
    network projects to the first cell.
    """

    def __init__(
        self, N=5, stim_w=0.04, stim_t=9, stim_delay=1, syn_w=0.01, syn_delay=5, r=50
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
        self._create_cells(r)  ### changed to use self._N instead of passing in N
        self._connect_cells()
        ### the 0th cell only exists on one process... that's the only one that gets a netstim
        if pc.gid_exists(0):
            self._netstim = h.NetStim()
            self._netstim.number = 1
            self._netstim.start = stim_t
            self._nc = h.NetCon(
                self._netstim, pc.gid2cell(0).syn
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
        ### this method is different because we now must use ids instead of objects
        for target in self.cells:
            source_gid = (target._gid - 1 + self._N) % self._N
            nc = pc.gid_connect(source_gid, target.syn)
            nc.weight[0] = self._syn_w
            nc.delay = self._syn_delay
            target._ncs.append(nc)

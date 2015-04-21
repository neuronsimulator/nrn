from neuron import h
import ballandstick
import numpy
from itertools import izip
from neuronpy.util import spiketrain

class Ring:
    """A network of *N* ball-and-stick cells where cell n makes an
    excitatory synapse onto cell n + 1 and the last, Nth cell in the
    network projects to the first cell.
    """
    def __init__(self, N=5, stim_w=0.004, stim_spike_num=1, syn_w=0.01,
            syn_delay=5):
        """
        :param N: Number of cells.
        :param stim_w: Weight of the stimulus
        :param stim_spike_num: Number of spikes generated in the stimulus
        :param syn_w: Synaptic weight
        :param syn_delay: Delay of the synapse
        """
        self._N = N              # Total number of cells in the net
        self.cells = []          # Cells on this host
        self.nclist = []         # NetCon list on this host
        self.gidlist = []        # List of global identifiers on this host
        self.stim = None         # Stimulator
        self.stim_w = stim_w     # Weight of stim
        self.stim_spike_num = stim_spike_num  # Number of stim spikes
        self.syn_w = syn_w       # Synaptic weight
        self.syn_delay = syn_delay  # Synaptic delay
        self.t_vec = h.Vector()   # Spike time of all cells on this host
        self.id_vec = h.Vector()  # Ids of spike times on this host

        #### Make a new ParallelContext object
        self.pc = h.ParallelContext()

        self.set_numcells(N)  # Actually build the net -- at least the portion
                              # of cells on this host.

    def set_numcells(self, N, radius=50):
        """Create, layout, and connect N cells."""
        self._N = N
        self.set_gids() #### Used when creating and connecting cells
        self.create_cells()
        self.connect_cells()
        self.connect_stim()

    def set_gids(self):
        """Set the gidlist on this host."""
        self.gidlist = []
        #### Round-robin counting.
        #### Each host as an id from 0 to pc.nhost() - 1.
        for i in range(int(self.pc.id()), self._N, int(self.pc.nhost())):
            self.gidlist.append(i)

    def create_cells(self):
        """Create cell objects on this host and set their location."""
        self.cells = []
        N = self._N
        r = 50 # Radius of cell locations from origin (0,0,0) in microns

        for i in self.gidlist:
            cell = ballandstick.BallAndStick()
            # When cells are created, the soma location is at (0,0,0) and
            # the dendrite extends along the X-axis.
            # First, at the origin, rotate about Z.
            cell.rotateZ(i*2*numpy.pi/N)

            # Then reposition
            x_loc = float(numpy.sin(i*2*numpy.pi/N))*r
            y_loc = float(numpy.cos(i*2*numpy.pi/N))*r
            cell.set_position(x_loc, y_loc, 0)

            self.cells.append(cell)

            #### Tell this host it has this gid
            #### gids can be any integer, they just need to be unique.
            #### In this simple case, we set the gid to i.
            self.pc.set_gid2node(i, int(self.pc.id()))

            #### Means to tell the ParallelContext that this cell is
            #### a source for all other hosts. NetCon is temporary.
            nc = cell.connect2target(None)
            self.pc.cell(i, nc) # Associate the cell with this host and gid

            #### Record spikes of this cell
            self.pc.spike_record(i, self.t_vec, self.id_vec)

# OLD WAY
#    def connect_cells(self):
#        self.nclist = []
#        N = self._N
#        for i in range(N):
#            src = self.cells[i]
#            tgt_syn = self.cells[(i+1)%N].synlist[0]
#            nc = src.connect2target(tgt_syn)
#            nc.weight[0] = self.syn_w
#            nc.delay = self.syn_delay
#            nc.record(self.t_vec, self.id_vec, i)
#            self.nclist.append(nc)

    def connect_cells(self):
        """Connect cell n to cell n + 1."""
        self.nclist = []
        N = self._N
        for i in self.gidlist:
            src_gid = (i-1+N) % N
            tgt_gid = i
            if self.pc.gid_exists(tgt_gid):
                target = self.pc.gid2cell(tgt_gid)
                syn = target.synlist[0]
                nc = self.pc.gid_connect(src_gid, syn)
                nc.weight[0] = self.syn_w
                nc.delay = self.syn_delay
                self.nclist.append(nc)

    def connect_stim(self):
        """Connect a spike generator on the first cell in the network."""
        #### If the first cell is not on this host, return
        if not self.pc.gid_exists(0):
            return
        self.stim = h.NetStim()
        self.stim.number = self.stim_spike_num
        self.stim.start = 9
        self.ncstim = h.NetCon(self.stim, self.cells[0].synlist[0])
        self.ncstim.delay = 1
        self.ncstim.weight[0] = self.stim_w # NetCon weight is a vector.

    def get_spikes(self):
        """Get the spikes as a list of lists."""
        return spiketrain.netconvecs_to_listoflists(self.t_vec, self.id_vec)

    def write_spikes(self, file_name='out.spk'):
        """Append the spike output file with spikes on this host. The output
        format is the timestamp followed by a tab then the gid of the source
        followed by a newline.

        :param file_name: is the full or relative path to a spike output file.

        .. note::

            When parallelized, each process will write to the same file so it
            is opened in append mode. The order in which the processes write is
            arbitrary so while the spikes within the process may be ordered by
            time, the output file will be unsorted. A quick way to sort a file
            is with the bash command sort, which can be called after all
            processes have written the file with the following format::

                exec_cmd = 'sort -k 1n,1n -k 2n,2n ' + file_name + \
                        ' > ' + 'sorted_' + file_name
                os.system(exec_cmd)
        """
        for i in range(int(self.pc.nhost())):
            self.pc.barrier() # Sync all processes at this point
            if i == int(self.pc.id()):
                if i == 0:
                    mode = 'w' # write
                else:
                    mode = 'a' # append
                with open(file_name, mode) as spk_file: # Append
                    for (t, id) in izip(self.t_vec, self.id_vec):
                        spk_file.write('%.3f\t%d\n' %(t, id)) # timestamp, id
        self.pc.barrier()

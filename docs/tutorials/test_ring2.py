from neuron import h
from neuron.units import ms, mV
import matplotlib.pyplot as plt
from ring import Ring

ring = Ring()

pc = h.ParallelContext()
pc.set_maxstep(10 * ms)

t = h.Vector().record(h._ref_t)
h.finitialize(-65 * mV)
pc.psolve(100 * ms)

# send all spike time data to node 0
local_data = {cell._gid: list(cell.spike_times) for cell in ring.cells}
all_data = pc.py_alltoall([local_data] + [None] * (pc.nhost() - 1))

if pc.id() == 0:
    # combine the data from the various processes
    data = {}
    for process_data in all_data:
        data.update(process_data)
    # plot it
    plt.figure()
    for i, spike_times in data.items():
        plt.vlines(spike_times, i + 0.5, i + 1.5)
    plt.show()

pc.barrier()
pc.done()
h.quit()
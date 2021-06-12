from neuron import h
from neuron.units import ms, mV
import matplotlib.pyplot as plt
from ring import Ring

cell_to_plot = 0

ring = Ring()

pc = h.ParallelContext()
pc.set_maxstep(10 * ms)

t = h.Vector().record(h._ref_t)
h.finitialize(-65 * mV)
pc.psolve(100 * ms)

if pc.gid_exists(cell_to_plot):
    plt.figure()
    plt.plot(t, pc.gid2cell(cell_to_plot).soma_v)
    plt.show()

pc.barrier()
pc.done()
h.quit()

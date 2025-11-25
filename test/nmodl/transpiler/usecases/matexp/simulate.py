import numpy as np

from neuron import gui, h
from neuron.units import ms

class Cell:
    def __init__(self, mechanism):
        self.soma = h.Section()
        self.soma.nseg = 1
        self.soma.insert(mechanism)
        self.ic = h.IClamp(self.soma(0.5))
        self.ic.delay = 0
        self.ic.dur = 1e9
        self.ic.amp = 10
        self.v_hoc = h.Vector().record(self.soma(0.5)._ref_v)
        self.t_hoc = h.Vector().record(h._ref_t)

cnexp = Cell('hh_cnexp')
matexp = Cell('hh_matexp')

h.stdinit()
h.tstop = 100.0 * ms
h.run()

for cell in [cnexp, matexp]:
    cell.v = np.array(cell.v_hoc.as_numpy())
    cell.t = np.array(cell.t_hoc.as_numpy())

error = cnexp.v - matexp.v
max_err = np.max(np.abs(error))
rmse = np.std(error)

if False:
    import matplotlib.pyplot as plt
    plt.figure("cnexp vs matexp")
    plt.plot(cnexp.t, cnexp.v, label='cnexp')
    plt.plot(matexp.t, matexp.v, label='matexp')
    plt.legend()
    plt.show()

assert rmse < 1e-6
assert max_err < 1e-3

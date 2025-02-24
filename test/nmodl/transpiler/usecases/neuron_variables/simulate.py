import numpy as np

from neuron import h, gui
from neuron.units import ms

s = h.Section()

s.insert("NeuronVariables")
celsius_hoc = h.Vector().record(s(0.5)._ref_range_celsius_NeuronVariables)

h.stdinit()
h.tstop = 5.0 * ms
h.run()

celsius = np.array(celsius_hoc.as_numpy())
assert celsius[-1] == 6.3, f"{celsius[-1]=}"

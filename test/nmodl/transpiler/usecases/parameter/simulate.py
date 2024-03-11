import numpy as np

from neuron import h, gui
from neuron.units import ms

s = h.Section()

test_parameter_pp = h.test_parameter(s(0.5))

assert test_parameter_pp.x == 42.0

test_parameter_pp.x = 42.1
assert test_parameter_pp.x == 42.1

s.insert("NeuronVariables")
celsius_hoc = h.Vector().record(s(0.5)._ref_range_celsius_NeuronVariables)

h.stdinit()
h.tstop = 5.0 * ms
h.run()

celsius = np.array(celsius_hoc.as_numpy())
assert celsius[-1] == 6.3, f"{celsius[-1]=}"

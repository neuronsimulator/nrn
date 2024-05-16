import numpy as np

from neuron import h, gui
from neuron.units import ms

s = h.Section()

test_parameter_pp = h.test_parameter(s(0.5))

# Defaults set in the PARAMETER block:
assert test_parameter_pp.x == 42.0
assert test_parameter_pp.y == 0.0

# Assignable:
test_parameter_pp.x = 42.1
assert test_parameter_pp.x == 42.1

h.stdinit()
h.tstop = 5.0 * ms
h.run()

# Values (not) set during the INITIAL block:
assert test_parameter_pp.x == 42.1
assert test_parameter_pp.y == 43.0

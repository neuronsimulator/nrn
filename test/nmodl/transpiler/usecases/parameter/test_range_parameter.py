import numpy as np

from neuron import h, gui
from neuron.units import ms

s = h.Section()

pp = h.range_parameter(s(0.5))

# Defaults set in the PARAMETER block:
assert pp.x == 42.0
assert pp.y == 0.0

# Assignable:
pp.x = 42.1
assert pp.x == 42.1

h.stdinit()
h.tstop = 5.0 * ms
h.run()

# Values (not) set during the INITIAL block:
assert pp.x == 42.1
assert pp.y == 43.0

import numpy as np

from neuron import h, gui
from neuron.units import ms

nseg = 1
s = h.Section()
s.insert("constant_mod")

expected = 2.3
assert s(0.5).constant_mod.foo() == expected

h.stdinit()

assert s(0.5).constant_mod.foo() == expected

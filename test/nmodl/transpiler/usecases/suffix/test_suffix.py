import numpy as np

from neuron import h, gui
from neuron.units import ms

s = h.Section()
s.insert("no_suffix")

pp = h.point_suffix(s(0.5))

h.stdinit()

assert s(0.25).no_suffix.x == 42.0, f"{s(0.25).no_suffix.x=}"
assert pp.x == 42.0, f"{pp.x=}"

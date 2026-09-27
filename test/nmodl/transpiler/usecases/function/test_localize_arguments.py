import numpy as np

from neuron import h, gui


s = h.Section()
s.insert("localize_arguments")

h.finitialize()

assert s(0.5).localize_arguments.id_v(22.0) == 22.0
assert s(0.5).localize_arguments.id_nai(22.0) == 22.0
assert s(0.5).localize_arguments.id_ina(22.0) == 22.0
assert s(0.5).localize_arguments.id_x(22.0) == 22.0
assert s(0.5).localize_arguments.id_g(22.0) == 22.0
assert s(0.5).localize_arguments.id_s(22.0) == 22.0
assert s(0.5).localize_arguments.id_p(22.0) == 22.0

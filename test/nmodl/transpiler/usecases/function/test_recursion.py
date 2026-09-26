import math

import numpy as np
from neuron import gui
from neuron import h


s = h.Section(name="soma")
s.insert("recursion")

n = 6
actual = [s(0.5).recursion.fibonacci(i) for i in range(n)]
expected = [1, 1, 2, 3, 5, 8]

assert actual == expected, f"{actual} != {expected}"

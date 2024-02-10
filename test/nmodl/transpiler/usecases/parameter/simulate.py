from neuron import h

s = h.Section()

test_parameter_pp = h.test_parameter(s(0.5))

assert test_parameter_pp.x == 42.

test_parameter_pp.x = 42.1

assert test_parameter_pp.x == 42.1


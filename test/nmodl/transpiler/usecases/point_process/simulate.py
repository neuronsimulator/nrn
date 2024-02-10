from neuron import h

test_pp = h.test_pp()

assert test_pp.has_loc() == 0

nseg = 1

s = h.Section()

test_pp_s = h.test_pp(s(0.5))

assert test_pp_s.has_loc() == 1

assert test_pp_s.get_segment() == s(0.5)

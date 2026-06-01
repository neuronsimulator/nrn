from neuron import h

instance = h.pp()
assert instance.has_loc() == 0

nseg = 1

s = h.Section()
instance = h.pp(s(0.5))

assert instance.has_loc() == 1
assert instance.get_segment() == s(0.5)

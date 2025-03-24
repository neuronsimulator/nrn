from neuron import h

def add_two(x):
    return x + 2

vec = h.Vector([4, -1.5, 17])
vec.apply(add_two)
assert list(vec) == [6.0, 0.5, 19.0]

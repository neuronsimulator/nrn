from neuron import h

nseg = 5
s = h.Section()
s.nseg = nseg

s.insert("test_func_proc")

coords = [(0.5 + k) * 1.0 / nseg for k in range(nseg)]
values = [ 0.1 + k for k in range(nseg)]

for x in coords:
    s(x).test_func_proc.set_x_42()
    assert s(x).test_func_proc.x == 42

for x, value in zip(coords, values):
    s(x).test_func_proc.set_x_a(value)

for x, value in zip(coords, values):
    assert s(x).test_func_proc.x == value

for x, value in zip(coords, values):
    assert s(x).test_func_proc.x_plus_a(100.0) == 100.0 + value

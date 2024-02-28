from neuron import h

nseg = 5
s = h.Section()
s.nseg = nseg

point_processes = []
for k in range(nseg):
    x = (0.5 + k) * 1.0 / nseg
    point_processes.append(h.test_func_proc_pnt(s(x)))

for k in range(nseg):
    point_processes[k].set_x_42()

for k in range(nseg):
    assert point_processes[k].x == 42

for k in range(nseg):
    value = 0.1 + k
    point_processes[k].set_x_a(value)

for k in range(nseg):
    value = 0.1 + k
    assert point_processes[k].x == value
    assert point_processes[k].x_plus_a(1000.0) == 1000.0 + value

print([point_processes[k].x for k in range(nseg)])

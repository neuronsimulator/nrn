from neuron import h

s = h.Section()

pnt_proc = h.test_func_proc_pnt(s(0.5))

pnt_proc.set_x_42()

assert pnt_proc.x == 42

pnt_proc.set_x_a(13.7)

assert pnt_proc.x == 13.7

assert pnt_proc.get_a_42(42) == 84

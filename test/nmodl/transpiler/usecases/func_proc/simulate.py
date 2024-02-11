from neuron import h

s = h.Section()

s.insert("test_func_proc")

s(0.5).test_func_proc.set_x_42()

assert s(0.5).test_func_proc.x == 42

s(0.5).test_func_proc.set_x_a(13.7)

assert s(0.5).test_func_proc.x == 13.7

assert s(0.5).test_func_proc.get_a_42(42) == 84

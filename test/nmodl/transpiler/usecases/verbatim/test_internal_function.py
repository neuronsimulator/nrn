from neuron import h


def test_internal_function():
    s = h.Section()
    s.nseg = 4

    s.insert("internal_function")

    assert s.internal_function.g() == 42.0
    assert s.internal_function.f() == 42.0

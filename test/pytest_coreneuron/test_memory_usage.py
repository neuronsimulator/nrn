from neuron import h


def test_memory_usage_hoc():
    assert h("print_local_memory_usage()")


def test_memory_usage():
    h.print_local_memory_usage()

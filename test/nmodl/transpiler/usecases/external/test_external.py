from neuron import h, gui


def assert_equal(a, b):
    assert a == b, f"{a} != {b}"


def test_external():
    s = h.Section()
    s.nseg = 5

    s.insert("dst")
    s.insert("src")

    h.gbl_src = 321.0

    assert_equal(s(0.5).dst.get_gbl(), h.gbl_src)
    assert_equal(s(0.5).dst.get_param(), h.param_src)


if __name__ == "__main__":
    test_external()

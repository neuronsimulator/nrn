from neuron import h, gui


def check_basic_pointer(make_instance):
    s = h.Section()
    s.nseg = 2

    s.insert("basic_pointer")

    s025 = make_instance(s, 0.25)
    s075 = make_instance(s, 0.75)

    s025._ref_p1 = s025._ref_x1
    s025._ref_p2 = s025._ref_x2

    s075._ref_p1 = s(0.25)._ref_v
    s075._ref_p2 = s(0.75)._ref_v

    s025.x1 = 123.0
    s025.x2 = 0.123

    assert s025.read_p1() == s025.x1
    assert s025.read_p2() == s025.x2

    s(0.25).v = 83.9
    s(0.75).v = -83.9

    assert s075.read_p1() == s(0.25).v
    assert s075.read_p2() == s(0.75).v


def test_basic_pointer():
    check_basic_pointer(lambda s, x: s(x).basic_pointer)


def test_point_basic():
    check_basic_pointer(lambda s, x: h.point_basic(x))


if __name__ == "__main__":
    test_basic_pointer()
    test_point_basic()

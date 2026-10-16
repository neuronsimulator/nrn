from neuron import h, gui


def test_pointing():
    s = h.Section()
    s.nseg = 5
    s.insert("pointing")

    s(0.1)._ref_ptr_pointing = s(0.1)._ref_v

    assert s(0.1).pointing.is_valid()
    assert not s(0.9).pointing.is_valid()


if __name__ == "__main__":
    test_pointing()

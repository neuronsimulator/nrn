from neuron import h, gui


def test_locals():
    s = h.Section()
    s.insert("locals")

    assert s(0.5).locals.get_a() == 32.0
    assert s(0.5).locals.get_b() == 132.0


if __name__ == "__main__":
    test_locals()

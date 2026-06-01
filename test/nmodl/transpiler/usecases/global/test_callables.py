from neuron import h, gui


def test_function():
    s = h.Section()
    s.insert("threading_effects")

    h.finitialize()

    assert s(0.5).threading_effects.sum_arr() == 30.3


def test_procedure():
    s = h.Section()
    s.insert("threading_effects")

    h.finitialize()

    s(0.5).threading_effects.set_g_w(44.4)
    assert h.g_w_threading_effects == 44.4


if __name__ == "__main__":
    test_function()
    test_procedure()

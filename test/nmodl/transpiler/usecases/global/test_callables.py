from neuron import h, gui


def test_function():
    s = h.Section()
    s.insert("shared_global")

    h.finitialize()

    assert s(0.5).shared_global.sum_arr() == 30.3


def test_procedure():
    s = h.Section()
    s.insert("shared_global")

    h.finitialize()

    s(0.5).shared_global.set_g_w(44.4)
    assert h.g_w_shared_global == 44.4


if __name__ == "__main__":
    test_function()
    test_procedure()

from neuron import h, gui


def test_nothing():
    x = 15.0
    assert h.forty_two_plus_x(x) == 42.0 + x
    assert h.twice_forty_two_plus_x(x) == 2.0 * (42.0 + x)
    assert h.forty_three() == 43.0


if __name__ == "__main__":
    test_nothing()

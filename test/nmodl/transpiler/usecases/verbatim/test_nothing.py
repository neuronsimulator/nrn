from neuron import h, gui


def test_nothing():
    assert h.get_foo() == 42.0


if __name__ == "__main__":
    test_nothing()

from neuron import h


def test_valence():
    assert h.ion_charge("K_ion") == 222.0


if __name__ == "__main__":
    test_valence()

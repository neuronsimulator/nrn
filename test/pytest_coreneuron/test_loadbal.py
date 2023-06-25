from neuron import h
import sys


def test_is_ion():
    mt = h.MechanismType(0)

    # check for non-ion type
    mt.select("hh")
    assert mt.is_ion() == False

    # check for ion type
    mt.select("k_ion")
    assert mt.is_ion() == True


if __name__ == "__main__":
    test_is_ion()

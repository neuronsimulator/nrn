import numpy as np

from neuron import h, gui


def test_default_values():
    s = h.Section()
    s.insert("default_values")
    mech = s(0.5).default_values

    h.stdinit()

    assert mech.X == 2.0
    assert mech.Y == 0.0
    assert mech.Z == 7.0

    for i in range(3):
        assert mech.A[i] == 4.0

    assert mech.B[0] == 5.0
    assert mech.B[1] == 8.0


if __name__ == "__main__":
    test_default_values()

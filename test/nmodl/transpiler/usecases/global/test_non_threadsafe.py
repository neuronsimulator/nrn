import numpy as np

from neuron import h, gui
from neuron.units import ms


def test_non_threadsafe():
    nseg = 1

    s = h.Section()
    s.insert("non_threadsafe")
    s.nseg = nseg

    h.finitialize()

    instance = s(0.5).non_threadsafe

    # Check INITIAL values.
    assert instance.get_parameter() == 41.0
    assert instance.get_gbl() == 42.0
    assert instance.get_top_local() == 43.0

    # Check reassigning a value. Top LOCAL variables
    # are not exposed to HOC/Python.
    h.parameter_non_threadsafe = 32.1
    h.gbl_non_threadsafe = 33.2

    assert instance.get_parameter() == 32.1
    assert instance.get_gbl() == 33.2


if __name__ == "__main__":
    test_non_threadsafe()

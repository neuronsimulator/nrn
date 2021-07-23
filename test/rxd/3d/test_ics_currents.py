from math import pi

import pytest

from testutils import compare_data, tol


@pytest.fixture
def ics_example(neuron_instance):
    """A model to test 3D intracellular rxd is correctly handling NMODL
    currents.
    """

    h, rxd, data, save_path = neuron_instance
    rxd.set_solve_type(dimension=3)
    # create cell1 where `x` will be created and leak out
    cell1 = h.Section(name="cell1")
    cell1.pt3dclear()
    cell1.pt3dadd(-1, 0, 0, 1)
    cell1.pt3dadd(1, 0, 0, 1)
    cell1.nseg = 11
    cell1.insert("pump")

    # Where?
    # the intracellular spaces
    cyt = rxd.Region(h.allsec(), name="cyt", nrn_region="i", dx=1.0)

    model = (cell1, cyt)
    yield (neuron_instance, model)


def test_ics_currents(ics_example):
    """Test ics_example with fixed step methods"""

    (h, rxd, data, save_path), (cell1, cyt) = ics_example
    x = rxd.Species([cyt], name="x", d=1.0, charge=1, initial=0)
    h.finitialize(-65)
    h.continuerun(100)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ics_currents_cvode(ics_example):
    """Test ics_example with variable step methods"""

    (h, rxd, data, save_path), (cell1, cyt) = ics_example
    x = rxd.Species([cyt], name="x", d=1.0, charge=1, initial=0)
    h.CVode().active(True)

    h.finitialize(-65)
    h.continuerun(100)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ics_currents_initial(ics_example):
    """Test ics_example with initial values from NEURON"""

    (h, rxd, data, save_path), (cell1, cyt) = ics_example
    x = rxd.Species([cyt], name="x", d=1.0, charge=1)
    h.finitialize(-65)
    h.continuerun(100)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol

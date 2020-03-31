from math import pi

import pytest

from testutils import compare_data, tol


@pytest.fixture
def ics_example(neuron_instance):
    """A model to test 3D intracellular rxd is correctly handling NMODL 
       currents.
    """

    h, rxd, data = neuron_instance
    rxd.set_solve_type(dimension=3)
    # create cell1 where `x` will be created and leak out
    cell1 = h.Section(name='cell1')
    cell1.pt3dclear()
    cell1.pt3dadd(-20, 0, 0, 10)
    cell1.pt3dadd(-10, 0, 0, 10)
    cell1.nseg = 11
    cell1.insert('pump')

    # Where?
    # the intracellular spaces
    cyt = rxd.Region(
        h.allsec(),
        name='cyt',
        nrn_region='i',
        dx=1.0)

    # Who?
    x = rxd.Species([cyt], name='x', d=1.0, charge=1, initial=0)
    Xcyt = x[cyt]
    model = (
        cell1,
        cyt,
        x,
        Xcyt
    )
    yield (neuron_instance, model)


def test_ics_currents(ics_example):
    """Test ics_example with fixed step methods"""

    (h, rxd, data), model = ics_example
    h.finitialize(-65)
    h.continuerun(100)

    max_err = compare_data(data)
    assert max_err < tol


def test_ics_currents_cvode(ics_example):
    """Test ics_example with variable step methods"""

    (h, rxd, data), model = ics_example

    h.CVode().active(True)

    h.finitialize(-65)
    h.continuerun(100)

    max_err = compare_data(data)
    assert max_err < tol

# This tests for a regression in `psection`:
#  https://github.com/neuronsimulator/nrn/issues/2783

from neuron import gui
from neuron import h


def test_psection():
    s = h.Section()
    s.insert("psection_regression")

    h.setdata_psection_regression(0.5, sec=s)
    h.setRNG_psection_regression(1, 1)

    h.psection(s)
    s.psection()

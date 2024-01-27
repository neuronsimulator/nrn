import pytest
from .testutils import compare_data, tol


def test_parameter_reaction(neuron_instance):
    """Test a simple rxd.Reaction involving a rxd.Parameter"""
    h, rxd, data, save_path = neuron_instance
    sec = h.Section(name="sec")
    cyt = rxd.Region([sec], name="cyt", nrn_region="i")
    ca = rxd.Species(cyt, name="ca", charge=2, initial=0.001)
    buf = rxd.Parameter(cyt, name="buf", value=0.001)
    cabuf = rxd.Species(cyt, name="cabuf", initial=0)
    buffering = rxd.Reaction(ca + buf, cabuf, 0.02, 0.01)
    buffering2 = rxd.Reaction(ca + buf, cabuf, 0.0, 0.0)
    badrate = rxd.Rate(buf, -buf)
    h.finitialize(-65)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol

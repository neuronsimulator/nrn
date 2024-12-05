import pytest
from .testutils import compare_data, tol


def test_state_reaction(neuron_instance):
    """Test a simple rxd.Reaction involving a rxd.State"""
    h, rxd, data, save_path = neuron_instance
    sec = h.Section(name="sec")
    cyt = rxd.Region([sec], name="cyt", nrn_region="i")
    ca = rxd.Species(cyt, name="ca", charge=2, initial=0.001)
    cabuf = rxd.State(cyt, name="cabuf", initial=0)
    # buffering = rxd.Reaction(ca, cabuf, 0.02)
    # generated the test data with a forward reaction
    buffering = rxd.Reaction(cabuf < ca, 0.02)
    h.finitialize(-65)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol

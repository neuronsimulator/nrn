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


def test_state_reaction2(neuron_instance):
    """Test a simple rxd.Reaction involving a rxd.State

    Looks for exact match of forward and backward at one time point
    """
    h, rxd, data, save_path = neuron_instance
    sec = h.Section("sec")
    cyt = rxd.Region([sec], name="cyt", nrn_region="i")
    ca = rxd.Species(cyt, name="ca", charge=2, initial=0.001)
    cabuf = rxd.State(cyt, name="cabuf", initial=0)
    ca2 = rxd.Species(cyt, name="ca2", charge=2, initial=0.001)
    cabuf2 = rxd.State(cyt, name="cabuf2", initial=0)
    buffering2 = rxd.Reaction(ca2, cabuf2, 0.02)
    # generated the test data with a forward reaction
    buffering = rxd.Reaction(cabuf < ca, 0.02)
    h.finitialize(-65)
    h.continuerun(1)
    assert sec(0.5).ca2i == sec(0.5).cai

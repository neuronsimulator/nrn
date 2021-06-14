import pytest


def test_unicode_names(neuron_instance):
    """A test that species on a region that do not have a nrn_region can have
    an unicode name.
    """

    h, rxd, data, save_path = neuron_instance
    soma = h.Section(name="soma")
    cyt = rxd.Region(soma.wholetree(), name="cyt")
    α = rxd.Parameter(cyt, name="α", value=0.3)
    h.finitialize(-65)

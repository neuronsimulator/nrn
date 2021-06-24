import pytest


def test_region_sections(neuron_instance):
    """A test that regions and species do not keep sections alive"""

    h, rxd, data, save_path = neuron_instance
    soma = h.Section(name="soma")
    soma.L = 5
    soma.diam = 5
    cyt = rxd.Region(soma.wholetree(), name="cyt", dx=1)
    ca = rxd.Species(cyt, name="ca", charge=2)
    h.finitialize(-65)
    soma = None

    for sec in h.allsec():
        assert False


def test_section_removal(neuron_instance):
    """A test that deleted sections are removed from rxd"""

    h, rxd, data, save_path = neuron_instance
    soma = h.Section(name="soma")
    soma.L = 5
    soma.diam = 5
    rxd.set_solve_type(soma, dimension=3)
    cyt = rxd.Region(soma.wholetree(), name="cyt", dx=1)
    ca = rxd.Species(cyt, name="ca", charge=2)
    h.finitialize(-65)

    soma = h.Section(name="soma")
    h.finitialize(-65)

    assert not ca.nodes

import pytest


def test_ecs_init(neuron_instance):
    """A test that reinitialization of extracellular species works when
    sections are added to the extracellular region after initialization.
    """
    h, rxd, data, save_path = neuron_instance

    ecs = rxd.Extracellular(-55, -55, -55, 55, 55, 55, dx=33)
    ca = rxd.Species(ecs, name="ca", charge=2)

    h.finitialize(-70)

    soma = h.Section(name="soma")
    h.finitialize(-70)

    assert True

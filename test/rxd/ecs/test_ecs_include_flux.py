import pytest

from testutils import compare_data, tol


@pytest.fixture
def ecs_include_flux(neuron_instance):
    """A model using node.include_flux for the extracellular space.

    node.include_flux can take either a; integer, Python function or NEURON
    pointer. All three are tested here for extracellular rxd.
    """

    h, rxd, data, save_path = neuron_instance
    sec = h.Section(name="dend")
    # the extracellular space
    ecs = rxd.Extracellular(
        -55, -55, -55, 55, 55, 55, dx=33, volume_fraction=0.2, tortuosity=1.6
    )

    # Who?
    x = rxd.Species([ecs], name="x", d=0.0, charge=1, initial=0)

    def callbackfun():
        return 1000

    x[ecs].node_by_location(-40, 0, 0).include_flux(1000)

    x[ecs].node_by_location(0, 0, 0).include_flux(callbackfun)

    x[ecs].node_by_location(40, 0, 0).include_flux(sec(0.5)._ref_v)

    yield (neuron_instance, (sec, ecs, x, callbackfun))


def test_ecs_include_flux(ecs_include_flux):
    """Test ecs_include_flux with fixed step methods"""

    neuron_instance, model = ecs_include_flux
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_include_flux_cvode(ecs_include_flux):
    """Test ecs_include_flux with variable step methods"""

    neuron_instance, model = ecs_include_flux
    h, rxd, data, save_path = neuron_instance
    h.CVode().active(True)
    h.finitialize(1000)
    h.continuerun(10)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol

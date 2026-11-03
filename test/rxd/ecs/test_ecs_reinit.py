import pytest


@pytest.fixture
def simple_model(neuron_nosave_instance):
    """A simple rxd model with species and extracellular regions."""

    h, rxd, save_path = neuron_nosave_instance
    dend = h.Section(name="dend")
    dend.diam = 2
    dend.nseg = 5
    dend.L = 5
    ecs = rxd.Extracellular(-10, -10, -10, 10, 10, 10, dx=3)
    cyt = rxd.Region(dend, name="cyt", nrn_region="i")
    k = rxd.Species(
        [cyt, ecs],
        name="k",
        d=1,
        charge=1,
        initial=lambda nd: 140 if nd.region == cyt else 3,
    )
    # test initialization for parameters with and without names
    paramA = rxd.Parameter([ecs], name="paramA", initial=1)
    paramB = rxd.Parameter([ecs], initial=0)
    decay = rxd.Rate(k, -0.1 * k)
    model = (dend, cyt, ecs, k, paramA, paramB, decay)
    yield (neuron_nosave_instance, model)


def test_ecs_reinit(simple_model):
    """Test rxd.re_init updates extracellular node values from NEURON values"""

    neuron_nosave_instance, model = simple_model
    h, rxd, save_path = neuron_nosave_instance
    dend, cyt, ecs, k, paramA, paramB, decay = model
    h.finitialize(-65)
    dend(0.2).ko = 0
    rxd.re_init()
    assert k[ecs].nodes((0, 0, 0)).value == [0]
    assert dend(0.2).paramAo == 1


def test_ecs_reinit_cvode(simple_model):
    """Test rxd.re_init updates extracellular node values from NEURON segments
    with CVode"""

    neuron_nosave_instance, model = simple_model
    h, rxd, save_path = neuron_nosave_instance
    dend, cyt, ecs, k, paramA, paramB, decay = model
    h.CVode().active(True)
    h.finitialize(-65)
    dend(0.2).ko = 0
    rxd.re_init()
    assert k[ecs].nodes((0, 0, 0)).value == [0]
    assert dend(0.2).paramAo == 1

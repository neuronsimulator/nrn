import pytest


@pytest.fixture
def simple_model(neuron_nosave_instance):
    """A simple rxd model with species and regions and reactions."""

    h, rxd, save_path = neuron_nosave_instance
    dend = h.Section(name="dend")
    dend.diam = 2
    dend.nseg = 5
    dend.L = 5
    cyt = rxd.Region(dend, name="cyt", nrn_region="i")
    k = rxd.Species([cyt], name="k", d=1, charge=1, initial=140)
    paramA = rxd.Parameter([cyt], name="paramA", initial=1)
    paramB = rxd.Parameter([cyt], initial=0)
    decay = rxd.Rate(k, -0.1 * k)
    model = (dend, cyt, k, paramA, paramB, decay)
    yield (neuron_nosave_instance, model)


def test_reinit(simple_model):
    """Test rxd.re_init updates node values from NEURON values"""

    neuron_nosave_instance, model = simple_model
    h, rxd, save_path = neuron_nosave_instance
    dend, cyt, k, paramA, paramB, decay = model
    h.finitialize(-65)
    dend(0.5).ki = 0
    rxd.re_init()
    assert k[cyt].nodes(dend(0.5)).value[0] == 0


def test_reinit_cvode(simple_model):
    """Test rxd.re_init updates node values from NEURON values with CVode"""

    neuron_nosave_instance, model = simple_model
    h, rxd, save_path = neuron_nosave_instance
    dend, cyt, k, paramA, paramB, decay = model
    h.finitialize(-65)
    h.CVode().active(True)
    dend(0.5).ki = 0
    rxd.re_init()
    assert k[cyt].nodes(dend(0.5)).value[0] == 0


def test_reinit_3d(simple_model):
    """Test rxd.re_init updates node values from NEURON values in 3D"""

    neuron_nosave_instance, model = simple_model
    h, rxd, save_path = neuron_nosave_instance
    dend, cyt, k, paramA, paramB, decay = model
    rxd.set_solve_type(dimension=3)
    # check changing the units after initialization
    h.finitialize(-65)
    dend(0.5).ki = 0
    rxd.re_init()
    for nd in k[cyt].nodes(dend(0.5)):
        assert nd.value == 0


def test_reinit_3d_cvode(simple_model):
    """Test rxd.re_init updates node values from NEURON values in 3D with
    CVode"""

    neuron_nosave_instance, model = simple_model
    h, rxd, save_path = neuron_nosave_instance
    dend, cyt, k, paramA, paramB, decay = model
    rxd.set_solve_type(dimension=3)
    h.CVode().active(True)
    # check changing the units after initialization
    h.finitialize(-65)
    dend(0.5).ki = 0
    rxd.species._has_1d = False
    rxd.re_init()
    for nd in k[cyt].nodes(dend(0.5)):
        assert nd.value == 0

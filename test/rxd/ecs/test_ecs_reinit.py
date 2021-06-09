import pytest

@pytest.fixture
def simple_model(neuron_instance):
    """A simple rxd model with species and extracellular regions."""

    h, rxd, data, save_path = neuron_instance
    dend = h.Section(name='dend')
    dend.diam = 2
    dend.nseg = 5 
    dend.L = 5
    ecs = rxd.Extracellular(-10, -10, -10, 10, 10, 10, dx=3)
    cyt = rxd.Region(dend, name='cyt', nrn_region='i')
    k = rxd.Species([cyt, ecs], name='k', d=1, charge=1,
                    initial=lambda nd: 140 if nd.region == cyt else 3)
    decay = rxd.Rate(k, -0.1*k)
    model = (dend, cyt, ecs, k, decay)
    yield (neuron_instance, model)

def test_ecs_reinit(simple_model):
    """Test rxd.re_init updates extracellular node values from NEURON values"""

    neuron_instance, model = simple_model
    h, rxd, data, save_path = neuron_instance
    dend, cyt, ecs, k, decay = model
    h.finitialize(-65)
    dend(0.2).ko = 0
    rxd.re_init()
    assert(k[ecs].nodes((0,0,0)).value == [0])

def test_ecs_reinit_cvode(simple_model):
    """Test rxd.re_init updates extracellular node values from NEURON segments
       with CVode """

    neuron_instance, model = simple_model
    h, rxd, data, save_path = neuron_instance
    dend, cyt, ecs, k, decay = model
    h.CVode().active(True)
    h.finitialize(-65)
    dend(0.2).ko = 0
    rxd.re_init()
    assert(k[ecs].nodes((0,0,0)).value == [0])

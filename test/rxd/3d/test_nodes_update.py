import pytest

@pytest.fixture
def ics_example(neuron_instance):
    """A simple 3D model to test nseg changes"""

    h, rxd, data, save_path = neuron_instance
    rxd.set_solve_type(dimension=3)
    soma = h.Section(name='soma')
    soma.L = 5
    soma.diam = 5
    soma.nseg = 5
    r = rxd.Region([soma], nrn_region='i', dx=0.5)
    c = rxd.Species(r, initial=1.0)
    model = (soma, r, c)
    yield (neuron_instance, model)

def test_nodes_update(ics_example):
    """Test nseg changes node for a 3D species"""

    (h, rxd, data, save_path), (soma, r, c) = ics_example
    assert(len({str(nd.segment) for nd in c.nodes}) == 5)
    soma.nseg = 3
    assert(len({str(nd.segment) for nd in c.nodes}) == 3)
    soma.nseg = 2
    assert(len({str(nd.segment) for nd in c.nodes}) == 2)


def test_nodes_update_initialized(ics_example):
    """Test nseg changes node for a 3D species after initialization"""

    (h, rxd, data, save_path), (soma, r, c) = ics_example
    h.finitialize(-65)
    assert(len({str(nd.segment) for nd in c.nodes}) == 5)
    soma.nseg = 3
    assert(len({str(nd.segment) for nd in c.nodes}) == 3)
    soma.nseg = 2
    assert(len({str(nd.segment) for nd in c.nodes}) == 2)


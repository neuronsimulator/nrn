import pytest
import numpy
from testutils import compare_data, tol


@pytest.fixture
def ics_pure_diffusion(neuron_instance):
    """A model using intracellular diffusion in a single section in 3D"""

    h, rxd, data, save_path = neuron_instance
    dend = h.Section(name="dend")
    dend.diam = 2
    dend.nseg = 11
    dend.L = 5
    rxd.set_solve_type(dimension=3)
    diff_constant = 1
    r = rxd.Region(h.allsec(), dx=0.75)
    ca = rxd.Species(
        r, d=diff_constant, initial=lambda node: 1 if 0.4 < node.x < 0.6 else 0
    )
    model = (dend, r, ca)
    yield (neuron_instance, model)


def test_pure_diffusion_3d(ics_pure_diffusion):
    """Test ics_pure_diffusion with fixed step methods"""

    neuron_instance, model = ics_pure_diffusion
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = model
    h.dt *= 50
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
    max_err = compare_data(data)
    if not save_path:
        assert max_err < tol


def test_pure_diffusion_3d_cvode(ics_pure_diffusion):
    """Test ics_pure_diffusion with variable step methods"""

    neuron_instance, model = ics_pure_diffusion
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = model
    h.CVode().active(True)
    vec = h.Vector()
    h.CVode().states(vec)
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
    max_err = compare_data(data)
    if not save_path:
        assert max_err < tol


def test_pure_diffusion_3d_inhom(ics_pure_diffusion):
    """Test ics_pure_diffusion with fixed step methods and inhomogeneous
    diffusion coefficients.
    """
    neuron_instance, model = ics_pure_diffusion
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = model
    h.dt *= 50
    for nd in ca.nodes:
        if nd.x >= 0.5:
            nd.d = 0
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
    max_err = compare_data(data)
    if not save_path:
        assert max_err < tol


def test_pure_diffusion_3d_inhom_cvode(ics_pure_diffusion):
    """Test ics_pure_diffusion with variable step methods and inhomogeneous
    diffusion coefficients.
    """
    neuron_instance, model = ics_pure_diffusion
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = model
    h.CVode().active(True)
    for nd in ca.nodes:
        if nd.x >= 0.5:
            nd.d = 0
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
    max_err = compare_data(data)
    if not save_path:
        assert max_err < tol

import pytest
import numpy
from testutils import compare_data, tol


@pytest.fixture
def ics_diffusion_hybrid(neuron_instance):
    """A model using intracellular diffusion in a 1D and 3D sections"""

    h, rxd, data, save_path = neuron_instance
    dend1 = h.Section(name="dend1")
    dend1.diam = 2
    dend1.nseg = 1
    dend1.L = 10

    dend2 = h.Section(name="dend2")
    dend2.diam = 2
    dend2.nseg = 11
    dend2.L = 10

    dend3 = h.Section(name="dend3")
    dend3.diam = 2
    dend3.nseg = 1
    dend3.L = 10

    dend2.connect(dend1)
    dend3.connect(dend2)

    diff_constant = 1

    r = rxd.Region(h.allsec(), dx=0.75)

    rxd.set_solve_type([dend2], dimension=3)

    ca = rxd.Species(
        r,
        d=diff_constant,
        initial=lambda node: (
            1
            if (0.8 < node.x and node in dend1) or (node.x < 0.2 and node in dend2)
            else 0
        ),
    )
    model = ([dend1, dend2, dend3], r, ca)
    yield (neuron_instance, model)


def test_pure_diffusion_hybrid(ics_diffusion_hybrid):
    """Test ics_diffusion_hybrid with fixed step methods"""

    neuron_instance, model = ics_diffusion_hybrid
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
        assert max_err < tol


def test_pure_diffusion_hybrid_cvode(ics_diffusion_hybrid):
    """Test ics_diffusion_hybrid with variable step methods"""

    neuron_instance, model = ics_diffusion_hybrid
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = model
    h.CVode().active(True)
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
        max_err = compare_data(data)
        assert max_err < tol


def test_pure_diffusion_hybrid_small_grid(ics_diffusion_hybrid):
    """Test ics_diffusion_hybrid with fixed step methods where 1D sections are
    outside the 3D grid
    """

    neuron_instance, model = ics_diffusion_hybrid
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = model
    dend[1].diam = 0.75

    h.dt *= 50
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
        max_err = compare_data(data)
        assert max_err < tol

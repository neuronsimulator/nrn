import pytest
import numpy
from math import exp
from testutils import compare_data, tol


@pytest.fixture
def ics_pure_diffusion_anisotropic(neuron_instance):
    """A model using intracellular diffusion in a single section in 3D"""

    def make_test(diff_constant):
        h, rxd, data, save_path = neuron_instance
        dend = h.Section(name="dend")
        dend.nseg = 11
        dend.pt3dclear()
        dend.pt3dadd(-3, 0, 0, 3)
        dend.pt3dadd(3, 0, 0, 3)
        rxd.set_solve_type(dimension=3)
        r = rxd.Region(h.allsec(), dx=0.75)
        ca = rxd.Species(
            r,
            d=diff_constant,
            initial=lambda nd: exp(
                -((nd.x3d - 0.375) ** 2 + (nd.y3d - 0.375) ** 2 + (nd.z3d - 0.375) ** 2)
            ),
        )
        return (dend, r, ca)

    yield (neuron_instance, make_test)


def test_pure_diffusion_3d_anisotropic_x(ics_pure_diffusion_anisotropic):
    """Test anisotropic without diffusion in the x direction with fixed step
    methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test([0, 0.1, 0.1])
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


def test_pure_diffusion_3d_anisotropic_y(ics_pure_diffusion_anisotropic):
    """Test anisotropic without diffusion in the y direction with fixed step
    methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test([0.1, 0, 0.1])
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


def test_pure_diffusion_3d_anisotropic_z(ics_pure_diffusion_anisotropic):
    """Test anisotropic without diffusion in the z direction with fixed step
    methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test([0.1, 0.1, 0])
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


def test_pure_diffusion_3d_anisotropic_x_cvode(ics_pure_diffusion_anisotropic):
    """Test anisotropic without diffusion in the x direction with variable step
    methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test([0, 0.1, 0.1])
    h.CVode().active(True)
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
    max_err = compare_data(data)
    if not save_path:
        assert max_err < tol


def test_pure_diffusion_3d_anisotropic_y_cvode(ics_pure_diffusion_anisotropic):
    """Test anisotropic without diffusion in the y direction with variable step
    methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test([0.1, 0, 0.1])
    h.CVode().active(True)
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
    max_err = compare_data(data)
    if not save_path:
        assert max_err < tol


def test_pure_diffusion_3d_anisotropic_z_cvode(ics_pure_diffusion_anisotropic):
    """Test anisotropic without diffusion in the z direction with variable step
    methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test([0.1, 0.1, 0])
    h.CVode().active(True)
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
    max_err = compare_data(data)
    if not save_path:
        assert max_err < tol


def test_pure_diffusion_3d_anisotropic_x_inhom(ics_pure_diffusion_anisotropic):
    """Test inhomogeneous anisotropic diffusion without diffusion in the x
    direction with fixed step methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test(lambda nd, dr: 0 if dr == 0 else 0.1)
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


def test_pure_diffusion_3d_anisotropic_y_inhom(ics_pure_diffusion_anisotropic):
    """Test inhomogeneous anisotropic diffusion without diffusion in the y
    direction with fixed step methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test(lambda nd, dr: 0 if dr == 1 else 0.1)
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


def test_pure_diffusion_3d_anisotropic_z_inhom(ics_pure_diffusion_anisotropic):
    """Test inhomogeneous anisotropic diffusion without diffusion in the z
    direction with fixed step methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test(lambda nd, dr: 0 if dr == 2 else 0.1)
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


def test_pure_diffusion_3d_anisotropic_x_inhom_cvode(ics_pure_diffusion_anisotropic):
    """Test inhomogeneous anisotropic diffusion without diffusion in the x
    direction with variable step methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test(lambda nd, dr: 0 if dr == 0 else 0.1)
    h.CVode().active(True)
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
    max_err = compare_data(data)
    if not save_path:
        assert max_err < tol


def test_pure_diffusion_3d_anisotropic_y_inhom_cvode(ics_pure_diffusion_anisotropic):
    """Test inhomogeneous anisotropic diffusion without diffusion in the y
    direction with variable step methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test(lambda nd, dr: 0 if dr == 1 else 0.1)
    h.CVode().active(True)
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
    max_err = compare_data(data)
    if not save_path:
        assert max_err < tol


def test_pure_diffusion_3d_anisotropic_z_inhom_cvode(ics_pure_diffusion_anisotropic):
    """Test inhomogeneous anisotropic diffusion without diffusion in the z
    direction with fixed step methods"""

    neuron_instance, make_test = ics_pure_diffusion_anisotropic
    h, rxd, data, save_path = neuron_instance
    dend, r, ca = make_test(lambda nd, dr: 0 if dr == 2 else 0.1)
    h.CVode().active(True)
    h.finitialize(-65)
    loss = -(numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    h.continuerun(125)
    loss += (numpy.array(ca.nodes.concentration) * numpy.array(ca.nodes.volume)).sum()
    if not save_path:
        assert loss < tol
    max_err = compare_data(data)
    if not save_path:
        assert max_err < tol

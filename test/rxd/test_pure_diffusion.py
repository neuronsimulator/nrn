from .testutils import compare_data, tol
from platform import platform


def applearm():
    return "macOS-" in platform() and "-arm64-" in platform()


def test_pure_diffusion(neuron_instance):
    """Test 1D diffusion in a single section"""

    h, rxd, data, save_path = neuron_instance
    dend = h.Section()
    dend.diam = 2
    dend.nseg = 101
    dend.L = 100

    diff_constant = 1

    r = rxd.Region([dend])
    ca = rxd.Species(
        r, d=diff_constant, initial=lambda node: 1 if 0.4 < node.x < 0.6 else 0
    )

    h.finitialize(-65)

    for t in [25, 50, 75, 100, 125]:
        h.continuerun(t)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_pure_diffusion_cvode(neuron_instance):
    """Test 1D diffusion in a single section with the variable step method."""

    h, rxd, data, save_path = neuron_instance
    dend = h.Section()
    dend.diam = 2
    dend.nseg = 101
    dend.L = 100

    diff_constant = 1

    r = rxd.Region(h.allsec())
    ca = rxd.Species(
        r, d=diff_constant, initial=lambda node: 1 if 0.4 < node.x < 0.6 else 0
    )

    # enable CVode and set atol
    h.CVode().active(1)
    h.CVode().atol(1e-6)

    h.finitialize(-65)

    for t in [25, 50, 75, 100, 125]:
        h.continuerun(t)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < (2e-10 if applearm() else tol)

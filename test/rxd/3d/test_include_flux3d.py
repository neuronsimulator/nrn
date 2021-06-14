import pytest

from testutils import compare_data, tol


@pytest.fixture
def ics_include_flux(neuron_instance):
    """A model using node.include_flux for the 3d intracellular space.

    node.include_flux can take either a; integer, Python function or NEURON
    pointer. All three are tested here for 3d intracellular rxd.
    """

    h, rxd, data, save_path = neuron_instance
    sec = h.Section(name="sec")
    sec.L = 1
    sec.nseg = 11
    sec.diam = 2
    rxd.set_solve_type(dimension=3)

    cyt = rxd.Region(h.allsec(), name="cyt", nrn_region="i")
    ip3 = rxd.Species(
        cyt, name="ip3", initial=lambda nd: 1000 if nd.segment == sec(0.3) else 0
    )

    def callbackfun():
        return 1000

    for nd in ip3.nodes(sec(0.1)):
        nd.include_flux(1000)

    for nd in ip3.nodes(sec(0.5)):
        nd.include_flux(callbackfun)

    for nd in ip3.nodes(sec(0.9)):
        nd.include_flux(sec(0.3)._ref_ip3i)

    model = (sec, cyt, ip3, callbackfun)
    yield (neuron_instance, model)


def test_include_flux3d(ics_include_flux):
    """Test ics_include_flux with fixed step methods"""

    neuron_instance, model = ics_include_flux
    h, rxd, data, save_path = neuron_instance
    h.finitialize(-70)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_include_flux3d_cvode(ics_include_flux):
    """Test ics_include_flux with variable step methods"""

    neuron_instance, model = ics_include_flux
    h, rxd, data, save_path = neuron_instance
    h.CVode().active(True)
    h.finitialize(-70)
    h.continuerun(10)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol

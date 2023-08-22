import pytest
from neuron import units
from matplotlib import pyplot


def test_plt_variable(neuron_instance):
    """Test to make sure species with multiple regions is not plotted"""

    h, rxd, data, save_path = neuron_instance
    import plotly

    h.load_file("stdrun.hoc")

    dend1 = h.Section("dend1")
    dend2 = h.Section("dend2")
    dend2.connect(dend1(1))

    dend1.nseg = dend1.L = dend2.nseg = dend2.L = 11
    dend1.diam = dend2.diam = 2 * units.µm

    cyt = rxd.Region(dend1.wholetree(), nrn_region="i")
    cyt2 = rxd.Region(dend2.wholetree(), nrn_region="i")

    ca = rxd.Species(
        [cyt, cyt2],
        name="ca",
        charge=2,
        initial=0 * units.mM,
        d=1 * units.µm**2 / units.ms,
    )

    ca.nodes(dend1(0.5))[0].include_flux(1e-13, units="mmol/ms")

    h.finitialize(-65 * units.mV)
    h.continuerun(50 * units.ms)

    ps = h.PlotShape(False)

    # Expecting an error for matplotlib
    with pytest.raises(Exception, match="Please specify region for the species."):
        ps.variable(ca)
        ps.plot(pyplot)

    # Expecting an error for plotly
    with pytest.raises(Exception, match="Please specify region for the species."):
        ps.variable(ca)
        ps.plot(plotly)

    # Scenarios that should work
    ps.variable(ca[cyt])
    ps.plot(plotly)  # No error expected here

    ps.variable(ca[cyt])
    ps.plot(pyplot)  # No error expected here

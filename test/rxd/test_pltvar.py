import pytest
import plotly
from neuron import units
from matplotlib import pyplot


def test_plt_variable(neuron_instance):
    """Test to make sure species with multiple regions is not plotted"""

    h, rxd, _, _ = neuron_instance

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
    h.fadvance()

    ps = h.PlotShape(False)

    # Expecting an error for matplotlib
    with pytest.raises(Exception, match="Please specify region for the species."):
        ps.variable(ca)
        ps.plot(pyplot)

    # Expecting an error for plotly
    with pytest.raises(Exception, match="Please specify region for the species."):
        ps.variable(ca)
        ps.plot(plotly)

    cb = rxd.Species(
        [cyt],
        name="cb",
        charge=2,
        initial=0 * units.mM,
        d=1 * units.µm**2 / units.ms,
    )

    # Scenarios that should work
    ps.variable(ca[cyt])
    ps.plot(plotly)  # No error expected here

    ps.variable(ca[cyt])
    ps.plot(pyplot)  # No error expected here

    # Test plotting with only one region
    ps.variable(cb)
    ps.plot(plotly)  # No Error expected here

    ps.variable(cb)
    ps.plot(pyplot)  # No Error expected here

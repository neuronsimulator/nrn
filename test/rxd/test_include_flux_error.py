import pytest


def _model(h, rxd, ecs):
    sec = h.Section(name="sec")
    sec.nseg = 3
    if ecs:
        region = rxd.Extracellular(-55, -55, -55, 55, 55, 55, dx=33)
        x = rxd.Species(region, name="fluxerr", d=0.0, initial=0)
        return sec, x, x[region].node_by_location(0, 0, 0)
    region = rxd.Region([sec], nrn_region="i")
    x = rxd.Species(region, name="fluxerr", initial=0)
    return sec, x, x.nodes(sec(0.5))[0]


def _run(h, cvode):
    h.CVode().active(cvode)
    h.finitialize(-65)
    h.continuerun(1)


@pytest.mark.parametrize("ecs", [False, True], ids=["1d", "ecs"])
@pytest.mark.parametrize("cvode", [False, True], ids=["fixed", "cvode"])
def test_include_flux_callback_raises(neuron_nosave_instance, capfd, ecs, cvode):
    """An exception in an include_flux callback stops the run with an error."""
    h, rxd, save_path = neuron_nosave_instance
    model = _model(h, rxd, ecs)

    def callback():
        raise ValueError("flux callback failed on purpose")

    model[2].include_flux(callback)
    with pytest.raises(RuntimeError):
        _run(h, cvode)
    assert "flux callback failed on purpose" in capfd.readouterr().err


@pytest.mark.parametrize("ecs", [False, True], ids=["1d", "ecs"])
def test_include_flux_callback_not_a_number(neuron_nosave_instance, capfd, ecs):
    """A callback that returns something other than a number is an error too."""
    h, rxd, save_path = neuron_nosave_instance
    model = _model(h, rxd, ecs)
    model[2].include_flux(lambda: "1000")
    with pytest.raises(RuntimeError):
        _run(h, False)
    assert "did not return a number" in capfd.readouterr().err

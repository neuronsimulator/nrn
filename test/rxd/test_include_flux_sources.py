import warnings

import pytest


def _node_with_ref_flux(h, rxd, source):
    """1D species over three segments; the middle node gets the flux `source`."""
    sec = h.Section(name="sec")
    sec.nseg = 3
    region = rxd.Region([sec], nrn_region="i")
    x = rxd.Species(region, name="fluxsrc", initial=0)
    node = x.nodes(sec(0.5))[0]
    node.include_flux(source)
    return sec, x, node


def _one_step(h, node):
    h.CVode().active(False)
    h.finitialize(-65)
    before = node.concentration
    h.fadvance()
    return node.concentration - before


def test_number_and_references_give_the_same_flux(neuron_nosave_instance):
    """A number, h.ref and a Vector element feed a node identically, and are read live."""
    h, rxd, save_path = neuron_nosave_instance
    deltas = {}
    sec, x, node = _node_with_ref_flux(h, rxd, 750.0)
    deltas["number"] = _one_step(h, node)
    ref = h.ref(750.0)
    vec = h.Vector([0.0, 750.0])
    sec2 = h.Section(name="sec2")
    sec2.nseg = 3
    region = rxd.Region([sec2], nrn_region="i")
    y = rxd.Species(region, name="fluxsrc2", initial=0)
    ny = y.nodes(sec2(0.5))[0]
    ny.include_flux(ref)
    deltas["ref"] = _one_step(h, ny)
    ref[0] = 1500.0
    deltas["ref doubled"] = _one_step(h, ny)

    sec3 = h.Section(name="sec3")
    sec3.nseg = 3
    region = rxd.Region([sec3], nrn_region="i")
    z = rxd.Species(region, name="fluxsrc3", initial=0)
    nz = z.nodes(sec3(0.5))[0]
    nz.include_flux(vec._ref_x[1])
    deltas["vector"] = _one_step(h, nz)

    assert deltas["number"] > 0
    assert deltas["ref"] == pytest.approx(deltas["number"], rel=1e-9, abs=0)
    assert deltas["vector"] == pytest.approx(deltas["number"], rel=1e-9, abs=0)
    assert deltas["ref doubled"] == pytest.approx(2 * deltas["number"], rel=1e-9, abs=0)


def test_flux_added_after_a_run(neuron_nosave_instance):
    """include_flux after a run rebuilds the sources; the earlier ones still work."""
    h, rxd, save_path = neuron_nosave_instance
    sec, x, first = _node_with_ref_flux(h, rxd, 400.0)
    one = _one_step(h, first)
    second = x.nodes(sec(0.1))[0]
    ref = h.ref(800.0)
    second.include_flux(ref)
    h.finitialize(-65)
    c1, c2 = first.concentration, second.concentration
    h.fadvance()
    assert first.concentration - c1 == pytest.approx(one, rel=1e-9, abs=0)
    assert second.concentration - c2 > 0


def test_reference_to_deleted_section_is_an_error(neuron_nosave_instance):
    """A reference that has become invalid must not turn into a NULL source.

    A NULL source means 'evaluate a callable', so the invalid reference used to run
    the last callable that was registered.
    """
    h, rxd, save_path = neuron_nosave_instance
    calls = []

    def callback():
        calls.append(1)
        return 1000.0

    sec, x, node = _node_with_ref_flux(h, rxd, 0.0)
    other = x.nodes(sec(0.1))[0]
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        other.include_flux(callback)
    gone = h.Section(name="gone")
    node.include_flux(gone(0.5)._ref_v)
    h.delete_section(sec=gone)
    h.CVode().active(False)
    with pytest.raises(RuntimeError):
        h.finitialize(-65)
        h.fadvance()
    assert calls == []


def test_vector_is_not_a_flux_reference(neuron_nosave_instance):
    """An unsupported HocObject source is an RxDException when it is given."""
    h, rxd, save_path = neuron_nosave_instance
    sec = h.Section(name="sec")
    sec.nseg = 3
    region = rxd.Region([sec], nrn_region="i")
    x = rxd.Species(region, name="fluxsrc", initial=0)
    node = x.nodes(sec(0.5))[0]
    with pytest.raises(rxd.RxDException):
        node.include_flux(h.Vector(3))


def test_failed_rebuild_keeps_the_previous_sources(neuron_nosave_instance):
    """If building the new sources fails, what the C code holds must still be backed by Python."""
    h, rxd, save_path = neuron_nosave_instance
    from neuron.rxd import node as node_module
    from neuron.rxd import rxd as rxd_module

    sec, x, node = _node_with_ref_flux(h, rxd, 1000.0)
    _one_step(h, node)
    constants = rxd_module._node_flux_constants
    callables = list(rxd_module._node_flux_callables)
    values = constants.copy()

    # an include_flux that passed its checks but fails when the sources are built
    class Unsupported:
        pass

    node_module._node_fluxes["source"].append(Unsupported())
    node_module._node_fluxes["index"].append(node._index)
    node_module._node_fluxes["type"].append(-1)
    node_module._node_fluxes["scale"].append(1.0)
    node_module._node_fluxes["region"].append(None)
    try:
        with pytest.raises(rxd.RxDException):
            rxd_module._include_flux(True)
    finally:
        for key in ("source", "index", "type", "scale", "region"):
            node_module._node_fluxes[key].pop()
    assert rxd_module._node_flux_constants is constants
    assert (rxd_module._node_flux_constants == values).all()
    assert rxd_module._node_flux_callables == callables

def test_arith_include_flux(neuron_nosave_instance):
    diff = 1e-15
    h, rxd, _ = neuron_nosave_instance
    dend1 = h.Section("dend1")
    dend2 = h.Section("dend2")
    dend1.nseg = 5
    dend2.nseg = 5

    cyt1 = rxd.Region(dend1.wholetree(), nrn_region="i")
    cyt2 = rxd.Region(dend2.wholetree(), nrn_region="i")
    ca1 = rxd.Species(cyt1, name="ca1", charge=2, initial=1e-12)
    ca2 = rxd.Species(cyt2, name="ca1", charge=2, initial=1e-12)

    node1 = ca1.nodes(dend1(0.5))[0]
    node2 = ca2.nodes(dend2(0.5))[0]
    node1.include_flux(ca1 * (1 - ca1))
    r = rxd.Rate(ca2, ca2 * (1 - ca2))

    h.finitialize(-65)
    h.dt /= 512
    h.continuerun(0.025 / 4)
    assert abs(node1.concentration - node2.concentration) < diff
    h.continuerun(0.025 / 2)
    assert abs(node1.concentration - node2.concentration) < diff
    h.continuerun(0.025)
    assert abs(node1.concentration - node2.concentration) < diff


def test_invalid_include_flux(neuron_nosave_instance):
    h, rxd, _ = neuron_nosave_instance
    dend1 = h.Section("dend1")
    cyt1 = rxd.Region(dend1.wholetree(), nrn_region="i")
    ca1 = rxd.Species(cyt1, name="ca1", charge=2, initial=1e-12)
    node1 = ca1.nodes(dend1(0.5))[0]
    try:
        node1.include_flux("wow")
    except Exception as e:
        assert isinstance(e, rxd.RxDException)
    try:
        node1.include_flux([])
    except Exception as e:
        assert isinstance(e, rxd.RxDException)
    try:
        node1.include_flux(None)
    except Exception as e:
        assert isinstance(e, rxd.RxDException)
    try:
        node1.include_flux({"a": 1})
    except Exception as e:
        assert isinstance(e, rxd.RxDException)

def test_nodelist_include_flux(neuron_nosave_instance):
    h, rxd, _ = neuron_nosave_instance
    dend1 = h.Section("dend1")
    diff = 1e-15
    cyt = rxd.Region(dend1.wholetree(), nrn_region="i")
    ca1 = rxd.Species(cyt, name="ca1", charge=2, initial=0)
    ca2 = rxd.Species(cyt, name="ca2", charge=2, initial=0)

    ca1.nodes(dend1(0.5)).include_flux(1e-16, units="mmol/ms")
    ca2.nodes(dend1(0.5))[0].include_flux(1e-16, units="mmol/ms")
    node1 = ca1.nodes(dend1(0.5))[0]
    node2 = ca2.nodes(dend1(0.5))[0]

    h.finitialize(-65)
    h.fadvance()
    assert abs(node1.concentration - 1.2732395447351626e-10) < diff
    assert abs(node2.concentration - 1.2732395447351626e-10) < diff
    h.fadvance()
    assert abs(node1.concentration - 2.546479089470325e-10) < diff
    assert abs(node2.concentration - 2.546479089470325e-10) < diff
    h.fadvance()
    assert abs(node1.concentration - 3.819718634205488e-10) < diff
    assert abs(node2.concentration - 3.819718634205488e-10) < diff
    h.fadvance()

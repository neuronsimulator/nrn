def test_region_volume(neuron_nosave_instance):
    h, rxd, _ = neuron_nosave_instance
    dend1 = h.Section("dend1")
    dend2 = h.Section("dend2")
    diff = 1e-10
    dend1.nseg = 4
    dend2.nseg = 4
    cyt1 = rxd.Region(dend1.wholetree(), "i", dx=0.25)
    cyt2 = rxd.Region(dend2.wholetree(), "i", dx=0.25)
    ca1 = rxd.Species(cyt1, name="ca1", charge=2, initial=1e-12)
    ca2 = rxd.Species(cyt2, name="ca2", charge=2, initial=1e-12)
    dend1.L = 10
    dend1.diam = 2
    dend2.L = 10
    dend2.diam = 2
    rxd.set_solve_type(domain=[dend1], dimension=1)
    rxd.set_solve_type(domain=[dend2], dimension=3)
    h.finitialize()
    assert abs(cyt1.volume() - sum(ca1.nodes(cyt1).volume)) < diff
    assert abs(cyt2.volume() - sum(ca2.nodes(cyt2).volume)) < diff

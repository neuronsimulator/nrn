def test_in(neuron_instance):
    """Test 1D diffusion in a single section"""

    h, rxd, data, save_path = neuron_instance

    soma = h.Section(name="soma")
    dend = h.Section(name="dend")
    soma.nseg = 3

    cyt = rxd.Region(h.allsec(), name="cyt")
    er = rxd.Region(h.allsec(), name="er")

    c = rxd.Species(cyt, name="c")

    for node in c.nodes(soma):
        assert node in soma
        assert node not in dend
        assert node in cyt
        assert node not in er

    for node in c.nodes(dend):
        assert node in dend
        assert node not in soma
        assert node in cyt
        assert node not in er


def test_in_segment(neuron_instance):
    """Test 1D diffusion in a single segment"""
    h, rxd, data, save_path = neuron_instance

    dend1 = h.Section("dend1")
    dend2 = h.Section("dend2")
    dend1.nseg = 4

    cyt1 = rxd.Region(dend1.wholetree(), nrn_region="i")
    ca1 = rxd.Species(cyt1, name="ca1", charge=2, initial=1e-12)

    node1 = ca1.nodes(dend1(0.5))[0]
    node2 = ca1.nodes(dend1(0.3))[0]

    assert node1 in dend1
    assert node1 not in dend2
    assert node1 not in dend1(0.125)
    assert node1 not in dend1(0.375)
    assert node1 in dend1(0.625)
    assert node1 not in dend1(0.875)

    assert node2 in dend1
    assert node2 not in dend2
    assert node2 not in dend1(0.125)
    assert node2 in dend1(0.375)
    assert node2 not in dend1(0.625)
    assert node2 not in dend1(0.875)

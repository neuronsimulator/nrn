def test_distance(neuron_instance):
    """Test 1D diffusion in a single section"""

    h, rxd, data, save_path = neuron_instance

    soma = h.Section(name="soma")
    dend = h.Section(name="dend")

    dend.connect(soma)

    cyt = rxd.Region([soma, dend], name="cyt")
    c = rxd.Species(cyt, name="c")

    assert h.distance(soma(0.5), dend(1)) == 150
    assert h.distance(soma(0.5), c.nodes[1]) == 100
    assert h.distance(c.nodes(soma), c.nodes(dend)) == 100

    h.distance(0, c.nodes(dend))
    assert h.distance(1, sec=dend) == 50

    assert h.distance(c.nodes[1], sec=soma) == 0
    assert h.distance(c.nodes(soma)) == 100

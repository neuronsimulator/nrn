def test_in(neuron_instance):
    """Test 1D diffusion in a single section"""

    h, rxd, data = neuron_instance
    
    soma = h.Section(name='soma')
    dend = h.Section(name='dend')
    soma.nseg = 3

    cyt = rxd.Region(h.allsec(), name='cyt')
    c = rxd.Species(cyt, name='c')

    for node in c.nodes(soma):
        assert(node in soma)
        assert(node not in dend)

    for node in c.nodes(dend):
        assert(node in dend)
        assert(node not in soma)


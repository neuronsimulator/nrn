def test_volumes_areas(neuron_instance):
    """Mode to test the 3D vocalization areas and volumes"""
    h, rxd, data, save_path = neuron_instance
    dend = h.Section(name="dend")
    dend.L = 2
    dend.diam = 2
    dend.nseg = 9
    dx = 0.25

    rxd.set_solve_type(dimension=3)
    cyt = rxd.Region([dend], name="cyt", dx=dx)
    ca = rxd.Species(cyt, name="ca", charge=2)

    nodes_on_surface = [node for node in ca.nodes if node.surface_area]
    nodes_internal = [node for node in ca.nodes if not node.surface_area]

    correct_surface_area = 15.713551302745485
    correct_surface_vol = 2.398148148148151
    correct_internal_vol = 3.0
    correct_vol = 5.398148148148163

    surface_area = sum([nd.surface_area for nd in nodes_on_surface])
    surface_vol = sum([nd.volume for nd in nodes_on_surface])
    internal_vol = sum([nd.volume for nd in nodes_internal])
    vol = sum([nd.volume for nd in ca.nodes])

    assert abs(correct_surface_area - surface_area) / correct_surface_area < 0.01
    assert abs(correct_surface_vol - surface_vol) / correct_surface_vol < 0.01
    assert abs(correct_internal_vol - internal_vol) / correct_internal_vol < 0.01
    assert abs(correct_vol - vol) / correct_vol < 0.01

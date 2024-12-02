import weakref


def test_node_selection(neuron_instance):
    """Test selection of 3D nodes"""

    h, rxd, data, save_path = neuron_instance

    rxd.set_solve_type(dimension=3)
    dend = h.Section(name="dend")
    dend.L = 10
    dend.diam = 2

    cyt = rxd.Region([dend])
    c = rxd.Species(cyt)

    nodes = c.nodes

    assert nodes((5.2, 0.1, 0.3))[0].x3d == 5.125
    assert nodes((5.2, 0.1, 0.3))[0].y3d == 0.125
    assert nodes((5.2, 0.1, 0.3))[0].z3d == 0.375

    nodes.append(rxd.node.Node3D(0, -1, 0, 0, cyt, 0, dend(0.5), weakref.ref(c)))

    assert len(nodes((5, 0, 0))) == 1
    assert nodes[-1] in nodes((nodes[-1].x3d, nodes[-1].y3d, nodes[-1].z3d))

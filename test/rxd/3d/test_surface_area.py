import numpy


def test_surface_area(neuron_instance):
    """Test that surface node all have surface area"""

    h, rxd, data, save_path = neuron_instance

    soma = h.Section(name="soma")
    soma.L = 1
    soma.diam = 1
    soma.nseg = 1

    rxd.set_solve_type(dimension=3)
    allsecs = soma.wholetree()
    r = rxd.Region(allsecs, dx=0.1)
    ca = rxd.Species(r)
    h.finitialize(-65)

    # true surface area
    sa = 1.5 * numpy.pi

    areas = numpy.array(ca.nodes.surface_area)

    # check surface area is 0.9 to 1.1 of the true value
    assert abs(1 - sum(areas) / sa) <= 0.1

    # check all surface nodes have non-zero surface area
    assert not any(areas[r._surface_nodes_by_seg[0]] == 0)

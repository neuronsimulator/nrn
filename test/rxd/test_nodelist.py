def test_only_nodes(neuron_instance):
    """Test to make sure node lists only contain nodes"""

    h, rxd, data, save_path = neuron_instance

    dend = h.Section("dend")
    r = rxd.Region(h.allsec())
    hydrogen = rxd.Species(r, initial=1)
    water = rxd.Species(r, initial=1)

    h.finitialize(-65)

    nodelist = hydrogen.nodes

    # test that should not work, so an append that succeeds is an error

    try:
        nodelist.append(water.nodes)  # append nodelist
        raise Exception("should not get here")
    except TypeError:
        ...

    try:
        nodelist.extend([1, 2, 3, water.nodes[0]])  # extend with non-nodes
        raise Exception("should not get here")
    except TypeError:
        ...

    try:
        nodelist[0] = 17
        raise Exception("should not get here")
    except TypeError:
        ...

    try:
        nl = rxd.nodelist.NodeList(
            [1, 2, 3, water.nodes[0]]
        )  # create NodeList with non-nodes
        raise Exception("should not get here")
    except TypeError:
        ...

    try:
        nodelist.insert(1, "llama")  # insert non-node into nodelist
        raise Exception("should not get here")
    except TypeError:
        ...

    # test that should work, so getting in the except is an error
    try:
        nodelist.append(water.nodes[0])  # append node
    except TypeError:
        raise Exception("should not get here")

    try:
        original_length = len(nodelist)  # extend nodes
        nodelist.extend(item for item in water.nodes)
        assert len(nodelist) == original_length + len(water.nodes)
    except TypeError:
        raise Exception("should not get here")

    try:
        nodelist[0] = water.nodes[0]
    except TypeError:
        raise Exception("should not get here")

    try:
        nl = rxd.nodelist.NodeList(
            [water.nodes[0], water.nodes[0]]
        )  # create nodelist with nodes
    except TypeError:
        raise Exception("should not get here")

    try:
        nodelist.insert(1, water.nodes[0])  # insert node into nodelist
    except TypeError:
        raise Exception("should not get here")

    try:
        nl = rxd.nodelist.NodeList([])  # create empty nodelist
    except TypeError:
        raise Exception("should not get here")

    try:
        nl = rxd.nodelist.NodeList(
            item for item in [water.nodes[0], water.nodes[0]]
        )  # create nodelist with nodes generator
        assert len(nl) == 2
    except TypeError:
        raise Exception("should not get here")

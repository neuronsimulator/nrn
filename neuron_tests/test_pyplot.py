from neuron import h


def test_mode(neuron_instance):
    """Test to make sure mode for show can be changed with and without interviews"""

    h = neuron_instance

    # PlotShape
    ps = h.PlotShape()

    for i in range(3):
        ps.show(i)
        if ps.show() != i:
            raise RuntimeError("PlotShape error")

    try:
        ps.show(3)
        raise Exception("ps.show should only take 0, 1, or 2")
    except RuntimeError:
        # RuntimeError is expected because ps.show(3) should fail
        ...

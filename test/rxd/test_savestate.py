def test_savestate(neuron_instance):
    """Test rxd SaveState

    Ensures restoring to the right point and getting the right result later.

    Note: getting the right result later was a problem in a prior version of
    NEURON, so the continuation test is important. The issue was that somehow
    restoring from a SaveState broke the rxd connection to the integrator.
    """

    h, rxd, data, save_path = neuron_instance

    soma = h.Section(name="soma")
    soma.insert(h.hh)
    soma.nseg = 51
    cyt = rxd.Region(h.allsec(), name="cyt")
    c = rxd.Species(cyt, name="c", d=1, initial=lambda node: 1 if node.x < 0.5 else 0)
    c2 = rxd.Species(
        cyt, name="c2", d=0.6, initial=lambda node: 1 if node.x > 0.5 else 0
    )
    r = rxd.Rate(c, -c * (1 - c) * (0.3 - c))
    r2 = rxd.Reaction(c + c2 > c2, 1)

    h.finitialize(-65)
    soma(0).v = -30

    while h.t < 5:
        h.fadvance()

    def get_state():
        return (
            soma(0.5).v,
            c.nodes(soma(0.5)).concentration[0],
            c2.nodes(soma(0.5)).concentration[0],
        )

    s1 = get_state()
    s = h.SaveState()
    s.save()

    while h.t < 10:
        h.fadvance()

    s2 = get_state()
    s.restore()

    assert get_state() == s1
    assert get_state() != s2

    while h.t < 10:
        h.fadvance()

    assert get_state() != s1
    assert get_state() == s2

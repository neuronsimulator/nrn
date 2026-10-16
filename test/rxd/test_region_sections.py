def test_region_sections(neuron_instance):
    """A test that regions and species do not keep sections alive"""

    h, rxd, data, save_path = neuron_instance
    soma = h.Section(name="soma")
    cyt = rxd.Region(soma.wholetree(), name="cyt")
    ca = rxd.Species(cyt, name="ca", charge=2)
    h.finitialize(-65)
    soma = None

    for sec in h.allsec():
        assert False


def test_section_removal(neuron_instance):
    """A test that deleted sections are removed from rxd"""

    h, rxd, data, save_path = neuron_instance
    soma = h.Section(name="soma")
    cyt = rxd.Region(soma.wholetree(), name="cyt")
    ca = rxd.Species(cyt, name="ca", charge=2)
    h.finitialize(-65)

    soma = h.Section(name="soma")
    h.finitialize(-65)

    assert not ca.nodes


def test_FractionalVolume_zero_surface_fraction_3d(neuron_instance):
    h, rxd, data, save_path = neuron_instance
    dend1 = h.Section("dend1")
    dend1.nseg = 5
    import warnings

    with warnings.catch_warnings(record=True) as w:
        cyt = rxd.Region(
            dend1.wholetree(),
            "i",
            name="cyt",
            geometry=rxd.FractionalVolume(volume_fraction=0.8),
            dx=0.25,
        )
        ca = rxd.Species(cyt, name="ca", charge=2, initial=1e-12)
        dend1.L = 10
        dend1.diam = 2
        rxd.set_solve_type(domain=[dend1], dimension=3)
        h.finitialize()
        assert len(w) == 1


def test_FractionalVolume_zero_surface_fraction_1d(neuron_instance):
    h, rxd, data, save_path = neuron_instance
    dend1 = h.Section("dend1")
    dend1.nseg = 5
    import warnings

    with warnings.catch_warnings(record=True) as w:
        cyt = rxd.Region(
            dend1.wholetree(),
            "i",
            name="cyt",
            geometry=rxd.FractionalVolume(volume_fraction=0.8),
            dx=0.25,
        )
        ca = rxd.Species(cyt, name="ca", charge=2, initial=1e-12)
        dend1.L = 10
        dend1.diam = 2
        rxd.set_solve_type(domain=[dend1], dimension=1)
        h.finitialize()
        assert len(w) == 1

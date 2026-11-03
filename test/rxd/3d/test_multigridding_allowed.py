from neuron.expect_hocerr import expect_hocerr


def should_not_work(h, rxd):
    """not allowed because regions with different dx neighbor"""
    soma1 = h.Section(name="soma1")
    dend1 = h.Section(name="dend1")
    soma1.L = dend1.L = 2
    soma1.diam = dend1.diam = 1
    dend1.connect(soma1)
    rxd.set_solve_type(dimension=3)
    cyt1 = rxd.Region([soma1], dx=0.2)
    cyt2 = rxd.Region([dend1], dx=0.25)
    ca = rxd.Species([cyt1, cyt2])

    h.finitialize(-65)
    h.fadvance()


def should_not_work2(h, rxd):
    """not allowed because regions with different dx overlap"""
    soma1 = h.Section(name="soma1")
    soma2 = h.Section(name="soma2")
    dend1 = h.Section(name="dend1")
    soma1.L = soma2.L = dend1.L = 2
    soma1.diam = soma2.diam = dend1.diam = 1
    dend1.connect(soma1)
    rxd.set_solve_type(dimension=3)
    cyt1 = rxd.Region(soma1.wholetree(), dx=0.2)
    cyt2 = rxd.Region([dend1, soma2], dx=0.25)
    ca = rxd.Species(cyt1)
    na = rxd.Species(cyt2)

    h.finitialize(-65)
    h.fadvance()


def should_work(h, rxd):
    """regions don't overlap so ok"""
    soma1 = h.Section(name="soma1")
    soma2 = h.Section(name="soma2")
    dend1 = h.Section(name="dend1")
    soma1.L = soma2.L = dend1.L = 2
    soma1.diam = soma2.diam = dend1.diam = 1
    dend1.connect(soma1)
    rxd.set_solve_type(dimension=3)
    cyt1 = rxd.Region(soma1.wholetree(), dx=0.2)
    cyt2 = rxd.Region([soma2], dx=0.25)
    ca = rxd.Species(cyt1)
    na = rxd.Species(cyt2)

    h.finitialize(-65)
    h.fadvance()


def should_work2(h, rxd):
    """regions overlap but have same dx, so no problem"""
    soma1 = h.Section(name="soma1")
    soma2 = h.Section(name="soma2")
    dend1 = h.Section(name="dend1")
    soma1.L = soma2.L = dend1.L = 2
    soma1.diam = soma2.diam = dend1.diam = 1
    dend1.connect(soma1)
    rxd.set_solve_type(dimension=3)
    cyt1 = rxd.Region(soma1.wholetree(), dx=0.25)
    cyt2 = rxd.Region([dend1, soma2], dx=0.25)
    ca = rxd.Species(cyt1)
    na = rxd.Species(cyt2)

    h.finitialize(-65)
    h.fadvance()


def test_no_overlap(neuron_nosave_instance):
    h, rxd, save_path = neuron_nosave_instance
    should_work(h, rxd)


def test_neighbors_with_different_dx_fails(neuron_nosave_instance):
    h, rxd, save_path = neuron_nosave_instance
    expect_hocerr(should_not_work, (h, rxd))


def test_overlapping_dx_fails(neuron_nosave_instance):
    h, rxd, save_path = neuron_nosave_instance
    expect_hocerr(should_not_work2, (h, rxd))


def test_overlap_same_dx(neuron_nosave_instance):
    h, rxd, save_path = neuron_nosave_instance
    should_work2(h, rxd)

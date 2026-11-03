from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(0)
from math import isclose


def test_nernst():
    assert h.nernst(1, 1, 0) == 0.0
    assert h.nernst(-1, 1, 1) == 1e6
    assert h.nernst(1, -1, 1) == -1e6

    s = h.Section()
    s.insert("hh")
    assert isclose(h.nernst("ena", sec=s), 63.55150321937486)
    assert isclose(h.nernst("ena", 0.5, sec=s), 63.55150321937486)
    assert isclose(h.nernst("nai", sec=s), 17.554820246530547)
    assert isclose(h.nernst("nao", sec=s), 79.7501757545304)
    expect_err('h.nernst("ina", sec=s)')

    del s
    locals()


def test_ghk():
    assert isclose(h.ghk(-10, 0.1, 10, 2), -2828.3285716150644)
    assert isclose(h.ghk(1e-6, 0.1, 10, 2), -1910.40949510667)


def test_ion_style():
    expect_err('h.ion_style("foo")')


def test_second_order_cur():
    s = h.Section()
    s.insert("hh")
    h.secondorder = 2
    h.finitialize(-65)
    h.fadvance()
    assert isclose(s(0.5).ina, -0.001220053188847315)
    h.secondorder = 0
    h.finitialize(-65)
    h.fadvance()
    assert isclose(s(0.5).ina, -0.0012200571764654333)


def test_ion_charge():
    assert h.ion_charge("na_ion") == 1
    expect_err('h.ion_charge("na") == 1')


def test_eion_cover():
    test_nernst()
    test_ghk()
    test_ion_style()
    test_second_order_cur()
    test_ion_charge()


if __name__ == "__main__":
    test_eion_cover()
    h.topology()

# see ../netrec_init.mod for the purpose of this test.

from neuron import h


def test_netrec_init():
    s = h.Section("soma")
    nri = h.NetRecInit(s(0.5))
    nri.foo = 2
    vref = s(0.5)._ref_v
    nri._ref_ptr = vref
    vref[0] = 5.0

    nc = h.NetCon(None, nri)
    nc.weight[1] = 11
    nc.weight[2] = 12

    h.finitialize()

    assert nc.weight[1] == nri.foo == 2.0
    assert nc.weight[2] == vref[0] == 5.0


if __name__ == "__main__":
    test_netrec_init()

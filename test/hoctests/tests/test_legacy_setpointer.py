# h.setpointer is deprecated in favor of h._ref_foo = h.ref_bar
# This test file started out life to cover nrnpy_setpointer_helper

from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)


def test_setpointer1():
    s = h.Section("soma")
    s.insert("sdatats")
    seg = s(0.5)
    mech = seg.sdatats
    vref = seg._ref_v
    h.setpointer(vref, "p", mech)
    seg.v = -10
    print(seg.v, mech.p, seg, mech)
    mech.p = 25
    print(seg.v, mech.p, seg, mech)
    return s


def test_setpointer2():
    s = h.Section("soma")
    s.insert("sdatats")
    seg = s(0.5)

    expect_err('h.setpointer(seg._ref_v, "foo", seg.sdatats)')
    expect_err('h.setpointer(seg._ref_v, "p", seg)')
    expect_err("h.setpointer(seg._ref_v, 5, seg.sdatats)")


if __name__ == "__main__":
    s = test_setpointer1()
    s = test_setpointer2()

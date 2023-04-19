# Cover ocpointer.cpp
# h.Pointer is unnecessary in Python as _ref_var suffices

from neuron import h
from neuron.expect_hocerr import expect_err


def test_ptr():
    h.t = 0
    ptr = h.Pointer("t", "x = $1*2")
    ptr.assign(5)
    assert h.t == 5.0
    assert h.x == 10.0
    assert ptr.s() == "t"
    ptr = h.Pointer("t", "x")
    h.x = 0
    ptr.assign(3)
    assert h.t == 3.0
    assert h.x == 3.0

    expect_err("ptr = h.Pointer('z')")

    s = h.Section(name="soma")
    s.nseg = 5
    ptr = h.Pointer(s(0.1)._ref_v)
    s(0.1).v = 25.0
    assert ptr.val == 25.0
    ptr.val = 26.1
    assert s(0.1).v == 26.1
    s.nseg = 1
    expect_err("ptr.val = 1.0")
    expect_err("ptr.assign(2.0)")
    return ptr


if __name__ == "__main__":
    ptr = test_ptr()

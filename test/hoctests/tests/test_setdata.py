from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)


def test_setdata(sfx):  #  "" or "ts"
    expect_err("h.A_sdata%s(5,6,7)" % sfx)  # No data for A_sdata.
    expect_err("h.setdata_sdata%s()" % sfx)  # not enough args
    expect_err("h.setdata_sdata%s(0.5)" % sfx)  # Section access unspecified

    fk = h.k_sdata if sfx == "" else h.k_sdatats  # does not use RANGE
    assert fk(4) == 10.0

    s = h.Section("s")
    s.insert("sdata%s" % sfx)
    s.nseg = 5

    setdata = h.setdata_sdata if sfx == "" else h.setdata_sdatats
    func = h.A_sdata if sfx == "" else h.A_sdatats

    for seg in s:
        setdata(seg)
        x = seg.x
        func(x, 10 * x, 100 * x)
        assert fk(3) == 6.0

    for seg in s:
        x = seg.x
        mech = seg.sdata if sfx == "" else seg.sdatats
        assert mech.a == x
        assert mech.b == 10 * x
        assert mech.c[1] == 100 * x

    # What happens if we setdata, call a function that sweeps over all
    # instances, and then call an instance function. We expect the value
    # in accordance with the last setdata.
    for seg in s:  # set a value that does not get changed by finitialize
        x = seg.x
        mech = seg.sdata if sfx == "" else seg.sdatats
        mech.c[2] = 1000 * x
    fc = h.C_sdata if sfx == "" else h.C_sdatats
    setdata(s(0.5))
    assert fc(2) == 1000 * 0.5
    h.finitialize()
    assert fc(2) == 1000 * 0.5

    del s, setdata, seg, mech, x
    locals()


# what happens if the _extcall_prop becomes invalid
def test_prop_invalid(sfx):  #  "" or "ts"
    s = h.Section("s2")
    s.insert("sdata%s" % sfx)
    s.nseg = 5
    setdata = h.setdata_sdata if sfx == "" else h.setdata_sdatats
    func = h.A_sdata if sfx == "" else h.A_sdatats
    fk = h.k_sdata if sfx == "" else h.k_sdatats  # does not use RANGE

    def set(x, y):
        seg = s(x)
        setdata(seg)
        func(y * 1, y * 10, y * 100)
        mech = seg.sdata if sfx == "" else seg.sdatats
        assert mech.c[1] == y * 100.0
        return seg

    set(0.5, 5)
    seg = set(0.1, 1)

    s.nseg = 1  # setdata invalid since old s2(0.1) no longer exists.
    mech = seg.sdata if sfx == "" else seg.sdatats
    expect_err("h.A_sdata%s(2, 20, 200)" % sfx)  #  Assertion failed: m_mech_handle
    assert mech.c[1] == 500.0  # Though seg.x == 0.1, the only point is 0.5
    assert fk(3) == 6.0
    carray = mech.c
    assert carray[1] == 500.0
    seg = set(0.5, 5)
    s.uninsert("sdata%s" % sfx)
    expect_err("print(carray[1])")  #  c_sdata, the mechanism does not exist at s2(0.1)
    expect_err("func(3, 30, 300)")  #  Assertion failed: m_mech_handle
    assert fk(4) == 10.0
    h.delete_section(sec=s)
    expect_err("print(carray[1])")  #  nrn.RangeVar can't access a deleted section
    expect_err("func(4, 40, 400)")  #  Assertion failed: m_mech_handle
    assert fk(5) == 15.0

    del s, setdata, seg, mech, carray, fk
    locals()


if __name__ == "__main__":
    for sfx in ["ts", ""]:
        test_setdata(sfx)
        test_prop_invalid(sfx)

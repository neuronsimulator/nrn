# test pp.func(...) and sec(x).mech.func(...)

from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet
from math import isclose

set_quiet(False)


def fc(name, m):  # callable
    return getattr(name, m)


def varref(name, m):  # variable reference
    return getattr("_ref_" + name, m)


def model():  # 3 cables each with nseg=3
    cables = [h.Section(name="cable%d" % i) for i in range(3)]
    mechs = []
    for c in cables:
        c.nseg = 3
        c.insert("sdata")
        c.insert("sdatats")
        for seg in c:
            mechs.append(h.SData(seg))
            mechs.append(h.SDataTS(seg))
            mechs.append(seg.sdata)
            mechs.append(seg.sdatats)
    return mechs  # cable section stay in existence since mechs ref them


def test1():
    print("test1")
    s = h.Section()  # so we can delete later and verify mechs is still ok
    s.nseg = 10
    mechs = model()

    def tst():
        for i, m in enumerate(mechs):
            m.A(i, 2 * i, 3 * i)
        for i, m in enumerate(mechs):
            assert m.a == float(i)
            assert m.b == float(2 * i)
            assert m.c[1] == float(3 * i)

    tst()
    h.finitialize()
    tst()
    del s
    tst()
    h.finitialize()
    tst()

    for sec in h.allsec():
        h.delete_section(sec=sec)
    for i in range(4):
        expect_err("mechs[i].A(0, 0, 0)")

    del mechs, sec, i
    locals()


def refs(mech):
    sec = mech.segment().sec
    seg = mech.segment()
    Af = mech.A
    aref = mech._ref_a
    return sec, seg, mech, Af, aref


def mech_expect_invalid(mech, Af, aref):
    expect_err("Af(0, 0, 0)")
    expect_err("aref[0] = 1.0")
    assert "died" in str(aref)
    assert str(mech) in [
        "<segment invalid or or mechanism uninserted>",
        "<mechanism of deleted section>",
    ]
    expect_err("print(mech.name())")
    del mech, Af, aref
    locals()


def test2():
    print("test2")
    mechs = model()
    sec, seg, mech, Af, aref = refs(mechs[-1])
    assert Af.name() == "sdatats.A"  # covers NPyMechObj_name
    expect_err("mech.Aexcept(5)")  # covers catch
    assert Af.mech() == mech  # covers NPyMechFunc_mech
    assert Af.__repr__() == "sdatats.A"  # covers pymechfunc_repr
    assert "A" in mech.__dict__
    # mech uninserted, should invalidate mechs[-1]
    mechs[-1].segment().sec.uninsert(mechs[-1].name())
    mech_expect_invalid(mech, Af, aref)
    del mechs, sec, seg, mech, Af, aref
    locals()


def test3():
    print("test3")
    mechs = model()
    sec, seg, mech, Af, aref = refs(mechs[-1])
    # internal segment destroyed, should invalidate mechs[-1]
    sec.nseg = 1
    mech_expect_invalid(mech, Af, aref)
    # seg exists but refers to the internal segment containing x
    assert seg.sec == sec
    assert seg.x == 5.0 / 6.0
    assert str(seg) == "cable2(0.833333)"
    del mechs, sec, seg, mech, Af, aref
    locals()


def test4():
    print("test4")
    mechs = model()
    sec, seg, mech, Af, aref = refs(mechs[-1])
    # section deleted, should invalidate mechs[-1]
    h.delete_section(sec=mechs[-1].segment().sec)
    mech_expect_invalid(mech, Af, aref)
    del mechs, sec, seg, mech, Af, aref
    locals()


def test5():
    print("test5")
    mechs = model()
    for sec in h.allsec():
        for seg in sec:
            for mech in seg:
                for rv in mech:
                    assert rv.mech() == mech
                    assert rv.mech().segment() == seg
                    assert rv.mech().segment().sec == sec


def test6():
    print("test6")
    mechs = model()
    m = mechs[0]
    expect_err("m.ft(1)")
    m.table_ft(5)
    assert m.ft(8) == 5.0

    vx = h.Vector([-100, 100])
    vy = vx.c().mul(2)
    m.table_ft(vy, vx)
    assert isclose(m.ft(5), 10.0)
    assert isclose(m.ft(10), 20.0)
    assert isclose(m.foo(10), m.ft(10))
    m.a = 3
    assert isclose(m.bar(), 2 * m.a)

    del mechs, m
    locals()


if __name__ == "__main__":
    test1()
    test2()
    test3()
    test4()
    test5()
    test6()
    h.topology()

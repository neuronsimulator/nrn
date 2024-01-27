from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

pc = h.ParallelContext()


def test_1():
    a = h.Section(name="axon")
    a.nseg = 5
    rv = {seg.x: seg._ref_v for seg in a.allseg()}

    def set(val):
        for seg in a.allseg():
            seg.v = val + 10.0 * seg.x

    def cmp(val):
        set(val)
        # test evaluation
        for x in rv:
            assert a(x).v == rv[x][0]
        # test assignment
        x = 0.5
        y = a(x).v * 2.0
        rv[x][0] = y
        assert a(x).v == y

    cmp(10)

    a.nseg *= 3  # simulations now 9 times more accurate spatially
    cmp(20)

    a.nseg = 5
    cmp(30)


def test_2():
    a = h.Section(name="axon")
    a.nseg = 5
    rv = {seg.x: seg._ref_v for seg in a.allseg()}
    h.finitialize(-65)
    assert rv[0.3][0] == -65.0
    expect_err("print(rv[0.3][1])")
    expect_err("rv[0.3][1] = 50.0")
    a.nseg = 3
    print(rv[0.3])
    expect_err("print(rv[0.3][0])")
    expect_err("rv[0.3][0] = 0.1")
    assert rv[0.5][0] == -65.0
    del a
    expect_err("print(rv[0.5][0])")

    del rv
    locals()


def test_3():
    a = h.Section(name="axon")
    a.nseg = 5
    pv = a(0.1)._ref_v
    a.nseg = 1
    v = h.Vector()
    expect_err("v.record(pv, v, sec=a)")
    del v, a
    locals()


def test_4():
    a = h.Section(name="axon")
    pnt = h.Pnt(0.5)
    expect_err("print(pnt.p)")
    expect_err("pnt.p = 5.0")
    pnt._ref_p = a(0.5)._ref_v
    a.v = 25
    print(pnt.p, a.v)
    print(pnt.p, a.v)
    print(pnt.p, a.v)
    a.nseg = 5
    pnt._ref_p = a(0.1)._ref_v
    print(pnt.p, a(0.1)._ref_v)
    a.nseg = 1
    expect_err("print(pnt.p, a(0.1).v)")

    del pnt, a
    locals()


def test_py2n_component():
    print("test py2n_component use of nrnpy_hoc_pop()")

    a = h.Section(name="axon")
    a.nseg = 5
    a.v = 50.0

    class Pntr:
        def __init__(self, ref):
            self.p = ref
            self.bas = [25]

        def foo(self, bar, bas):
            print(bar, bas)

    p = Pntr(a(0.1)._ref_v)
    assert p.p[0] == a(0.1).v
    print("p.p", p.p)
    h(
        r"""
proc tst_py2n_component1() {
    nseg = $3
    $o1.foo(&$&2, 1)
}

func tst_py2n_component2_nseg() {
    printf("inside tst_py2n_component2_nseg\n")
    nseg = $1
    return nseg
}

proc tst_py2n_component2() {
    printf("nseg = %d\n", nseg)
    nseg = 1
    $o1.foo(&$&2, tst_py2n_component2_nseg(1))
    printf("nseg = %d\n", nseg)
}

proc tst_py2n_component3() {
    nseg = 1
    printf("%g\n",$&1)
}

"""
    )

    a.nseg = 5
    h.tst_py2n_component1(p, a(0.1)._ref_v, 5, sec=a)
    vref = a(0.1)._ref_v
    print("Expect: arg 1 error: Invalid data handle")
    expect_err("h.tst_py2n_component1(p, vref, 1, sec=a)")
    print("Expect: Invalid pointer (arg 1)")
    expect_err("h.tst_py2n_component1(p, vref, 1, sec=a)")

    a.nseg = 5
    print("Expect: arg 1 error: Invalid data handle")
    expect_err("h.tst_py2n_component2(p, a(.1)._ref_v, sec=a)")

    a.nseg = 5
    print("Expect: hoc argument 1 is an invalid datahandle")
    expect_err("h.tst_py2n_component3(a(.1)._ref_v, sec=a)")

    del p, vref, a
    locals()
    print("leaving test_py2n_component")


def test_hocobj_getitem():
    v = h.Vector(5).indgen()
    assert v.x[2] == 2.0  # will call nrnpy_hoc_pxpop when VAR on stack
    # There appears no way to do that with a datahandle though
    a = h.Section(name="axon")
    h("objref test_hocobj_getitem_vref")
    h.test_hocobj_getitem_vref = a(0.5)._ref_v
    assert h.test_hocobj_getitem_vref is None


def test_array_dim_change():
    # not really a datapointer test, but...
    print("declare double array_dim_change[5][2]")
    h("double array_dim_change[5][2]")
    x2 = h.array_dim_change[3]
    print("declare double array_dim_change[10]")
    h("double array_dim_change[10]")
    expect_err("print(x2[1])")

    print("declare double array_dim_change[5][2]")
    h("double array_dim_change[5][2]")
    x2 = h.array_dim_change[4]
    print("x2 is ", x2)
    print("declare double array_dim_change[2][2]")
    h("double array_dim_change[2][2]")
    print("x2 is ", x2)
    expect_err("print('x2[1] is ', x2[1])")
    print("x2 is ", x2)
    expect_err("x2[1] = 5.0")

    del x2
    locals()
    print("leaving test_array_dim_change")


if __name__ == "__main__":
    set_quiet(False)
    test_1()
    test_2()
    test_3()
    test_4()
    test_py2n_component()
    test_hocobj_getitem()
    test_array_dim_change()
    h.topology()

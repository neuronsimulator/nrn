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


if __name__ == "__main__":
    set_quiet(False)
    test_1()
    test_2()
    test_3()
    h.topology()

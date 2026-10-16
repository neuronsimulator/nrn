from neuron import h


def test_random_play():  # for coverage of ptrlist changes #1815
    cv = h.CVode()
    cv.active(0)
    h.secondorder = 0
    s = h.Section()
    s.L = 10
    s.diam = 10
    ic = h.IClamp(s(0.5))
    ic.delay = 0
    ic.dur = 10
    ic.amp = 0
    r = h.Random()
    r.Random123(1, 0, 0)
    r.uniform(0.0, 0.001)
    r.play(ic._ref_amp)
    v_vec = h.Vector()
    v_vec.record(s(0.5)._ref_v, sec=s)
    h.dt = 0.025
    h.finitialize(0)
    for _ in range(3):
        h.fadvance()
    std = h.Vector(
        [0.0, 0.00046940111168614074, 0.004571699024450762, 0.01086603263672303]
    )
    assert v_vec.c().sub(std).abs().sum() < 1e-15


def test_hoc_list():  # for coverage of ptrlist changes in ivoc/oc_list.cpp
    lst = h.List()
    lst.append(h.Vector().append(0))
    lst.append(h.Vector().append(1))
    lst.insrt(1, h.Vector().append(2))
    assert [v.x[0] for v in lst] == [0.0, 2.0, 1.0]


def test_rvp():  # for coverage of ptrlist changes in nrniv/shape.cpp
    d = [h.Section(name="d_" + str(i)) for i in range(3)]
    for i in range(1, 3):
        d[i].connect(d[i - 1](1))
    for s in d:
        s.L = 10
        s.diam = 1
        s.insert("pas")
    rvp = h.RangeVarPlot("v", d[0](0), d[2](1))
    assert rvp.left() == 0.0
    assert rvp.right() == 30.0
    sl = h.SectionList()
    rvp.list(sl)
    sz = sum([1 for _ in sl])
    assert sz == 3


def test_finithnd():
    # ptrlist #1815 coverage of nrniv/finithnd.cpp
    import sys, io

    def foo():
        pass

    fih = h.FInitializeHandler(foo)
    oldstdout = sys.stdout
    sys.stdout = mystdout = io.StringIO()
    fih.allprint()
    sys.stdout = oldstdout
    assert "Type 1 FInitializeHandler statements" in mystdout.getvalue()


if __name__ == "__main__":
    test_random_play()
    test_hoc_list()
    test_rvp()
    test_finithnd()

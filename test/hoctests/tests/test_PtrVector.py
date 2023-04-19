# Coverage for  ocptrvector.cpp

from neuron import h, gui
from neuron.expect_hocerr import expect_err

pc = h.ParallelContext()


def test_ptrvector():
    s = h.Section(name="soma")
    s.nseg = 5
    pvec = h.PtrVector(3)
    assert pvec.size() == 3
    pvec.resize(5)
    assert pvec.size() == 5
    for i, seg in enumerate(s):
        pvec.pset(i, seg._ref_v)
    vec = h.Vector(pvec.size()).indgen().mul(0.2)
    for i, seg in enumerate(s):
        pvec.setval(i, vec[i])
        assert pvec.getval(i) == vec[i]
        assert pvec.getval(i) == seg.v
    vec.add(1)
    pvec.scatter(vec)
    for i, seg in enumerate(s):
        assert vec[i] == seg.v
        seg.v += 1
    pvec.gather(vec)
    std = vec.c()

    def std_eq_seg():
        for i, seg in enumerate(s):
            print(i, std[i], pvec.getval(i), seg.v)
            assert std[i] == seg.v
        for i, x in enumerate(std):
            assert x == pvec.getval(i)

    std_eq_seg()
    assert pvec.label() == ""
    pvec.label("segmentVoltages")
    assert pvec.label("Voltages") == "Voltages"

    g = h.Graph()
    vec.indgen()
    pvec.plot(g)
    pvec.plot(g, 2, 3)
    pvec.plot(g, vec)
    pvec.plot(g, vec, 3, 2)
    pvec.plot(g, 0.5)
    pvec.plot(g, 0.5, 4, 4)
    # too many arguments
    expect_err("pvec.plot(g, vec, 3, 2, 1)")

    # generate warning (no ptr update callback)
    s2 = h.Section()
    h.cvode.cache_efficient(1)
    h.finitialize()

    # hoc version of a callback
    h(
        """
begintemplate Foo
public callback, x
proc callback() {
    print "inside hoc callback"
    x = 1
}
endtemplate Foo
    """
    )
    foo = h.Foo()
    foo.x = 0
    pvec.ptr_update_callback("callback()", foo)
    pc.nthread(2)
    h.finitialize()
    assert foo.x == 1.0
    pc.nthread(1)

    # python version of a callback
    global x

    def callback():
        global x
        x = 2
        print("inside python callback x=", x)

    pvec.ptr_update_callback(callback)
    x = 0
    pc.nthread(2)
    h.finitialize()
    assert x == 2

    # former gives 100% coverage of ocptrvector.cpp
    std_eq_seg()  # only substantive test of data handles so far
    s.nseg = 4
    expect_err("std_eq_seg()")  # one of the datahandles is invalid

    h.PtrVector(10)  # create and destroy

    return g


if __name__ == "__main__":
    g = test_ptrvector()

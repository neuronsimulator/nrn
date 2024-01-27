from neuron import h, gui
from neuron.expect_hocerr import expect_err
import sys

pc = h.ParallelContext()


def callback(arg):
    print("callback: arg={}".format(arg))
    if arg == 1:
        print(1 / 0)
    if arg == 2:
        print("calling sys.exit()")
        sys.exit(0)


def test_finithandler():
    soma = h.Section(name="soma")
    fih = h.FInitializeHandler(1, (callback, (0)))
    for i in range(2):
        h.finitialize()

    fih = h.FInitializeHandler(1, (callback, (1)))
    for i in range(1):
        expect_err("h.finitialize()")

    del fih  # a mysterious consequence of expect_err is that need this
    locals()  # in order for the callback to be deleted in time for next
    if __name__ == "__main__":
        print("another test")
        fih = h.FInitializeHandler(1, (callback, (2)))
        h.finitialize()
        print("this line gets printed in interactive mode")


def efun(v):
    if v.x[0] == 100.0:
        print(1 / 0)
    if v.x[0] == 200.0:
        print("calling sys.exit()")
        sys.exit(0)
    e = v.sumsq()
    return e


def test_praxis():
    vec = h.Vector([2, 3])
    h.attr_praxis(1e-5, 0.5, 0)
    print("x={} vec={}".format(efun(vec), vec.to_python()))
    x = h.fit_praxis(efun, vec)
    print("after fit_praxis x={} vec={}".format(x, vec.to_python()))

    vec.x[0] = 100.0
    expect_err("h.fit_praxis(efun, vec)")
    if __name__ == "__main__":
        vec.x[0] = 200.0
        print("another test")
        x = h.fit_praxis(efun, vec)
        print("this line gets printed in interactive mode")


def foo(i):
    i = int(i)
    if i == 2:
        1 / 0
    if i == 3:
        sys.exit(0)
    return list(range(i))


def test_py2n_component():
    h("objref po")
    h.po = foo
    h("po = po._(1)")
    assert type(h.po) == type([])
    h.po = foo
    a = h("po = po._(2)")
    assert a == 0
    if __name__ == "__main__":
        h.po = foo
        h("po = po._(3)")
        print("this test_py2n_component line gets printed in interactive mode")
    h("objref po")


def test_func_call():
    for sec in h.allsec():
        h.delete_section(sec=sec)
    soma = h.Section(name="soma")
    dend = h.Section(name="dend")
    axon = h.Section(name="axon")
    dend.connect(soma(0))
    dend.nseg = 5
    axon.connect(soma(1))
    axon.nseg = 3
    h.topology()
    invoke = False

    def f(x):
        y = x**2
        if invoke:
            if h.cas() == dend and x == 1.0:
                1 / 0
            if h.cas() == dend and x > 0.6:
                sys.exit(0)
        print("{}({})".format(h.cas(), x))
        return y

    # all points exist
    rvp = h.RangeVarPlot(f, dend(1.0), axon(1.0))
    vec = h.Vector()
    rvp.to_vector(vec)
    invoke = True

    # but no errors as those are marked non-existent
    h.RangeVarPlot(f, dend(1.0), axon(1.0))

    # now an error
    expect_err("rvp.to_vector(vec)")

    if __name__ == "__main__":
        invoke = False
        rvp = h.RangeVarPlot(f, dend(0.9), axon(1.0))
        invoke = True
        rvp.to_vector(vec)
        print("this test_func_call line gets printed in interactive mode")

    del soma, dend, axon, rvp, vec
    locals()


def f(x):
    y = x * x
    if x == 4:
        y = 1.0 / 0.0
    return x, y


def test_call_picklef():
    print("test_call_picklef")
    pc.runworker()
    for i in range(6):
        pc.submit(f, float(i))
    try:
        while pc.working():
            x, y = pc.pyret()
            print("{} = f({})".format(y, x))
    except Exception as e:
        print(e)


if __name__ == "__main__":
    test_praxis()
    test_finithandler()
    test_py2n_component()
    test_call_picklef()
    test_func_call()

from neuron import h, gui
from neuron.expect_hocerr import expect_hocerr

pc = h.ParallelContext()


def run(tstop):
    h.tstop = tstop
    h.run()


def on_finit(ste, result):
    result.clear()
    ste.state(0)


def act(i, m, thresh, result):
    print(
        "act %d t=%g (%g) v=%g thresh=%g" % (i, h.t, pc.t(0), m["s"](0.5).v, thresh[0])
    )
    # allow verification of result
    result.append((i, thresh[0], m["vvec"].size() - 1))


def chk_result(sz, result, m):
    print(result)
    for x in result:
        th = x[1]
        i = x[2]
        vvec = m["vvec"]
        print(
            "vvec[%d]=%.18g th=%.18g vvec[%d]=%.18g"
            % (i - 1, vvec[i - 1], th, i, vvec[i])
        )
        assert vvec[i] >= th - h.float_epsilon and vvec[i - 1] <= th
    assert len(result) == sz


def model():
    # model with linear increase in voltage
    s = h.Section()
    s.L = 10
    s.diam = 10
    ic = h.IClamp(s(0.5))
    ic.delay = 0
    ic.dur = 10
    ic.amp = 0.1
    vvec = h.Vector().record(s(0.5)._ref_v, sec=s)
    tvec = h.Vector().record(h._ref_t, sec=s)
    return {"s": s, "ic": ic, "vvec": vvec, "tvec": tvec}


def test_ste():

    m1 = model()
    # one state ste with two self transitions
    var = m1["s"](0.5)._ref_v
    thresh1 = h.ref(-50)
    thresh2 = h.ref(-10)
    result = []
    ste = h.StateTransitionEvent(1)
    ste.transition(0, 0, var, thresh1, (act, (1, m1, thresh1, result)))
    ste.transition(0, 0, var, thresh2, (act, (2, m1, thresh2, result)))
    fih = h.FInitializeHandler((on_finit, (ste, result)))

    run(5)
    print("final v=%g" % m1["s"](0.5).v)
    chk_result(2, result, m1)

    h.cvode_active(1)
    run(20)
    chk_result(2, result, m1)

    h.cvode_active(1)
    h.cvode.condition_order(2)
    run(5)
    chk_result(2, result, m1)

    h.cvode.condition_order(1)
    h.cvode_active(0)

    # ste associated with point process
    del fih, ste
    ste = h.StateTransitionEvent(2, m1["ic"])
    fih = h.FInitializeHandler((on_finit, (ste, result)))
    run(5)

    # transition with hoc callback
    h("""proc foo() { printf("foo called at t=%g\\n", t) }""")
    thresh3 = h.ref(-30)
    ste.transition(0, 0, var, thresh3, "foo()")
    run(5)

    # transition with hoc callback in hoc object
    h(
        """
begintemplate FooSTEtest
objref this
proc foo() { printf("foo in %s called at t=%g\\n", this, t) }
endtemplate FooSTEtest
"""
    )
    thresh4 = h.ref(-20)
    obj = h.FooSTEtest()
    ste.transition(0, 0, var, thresh4, "foo()", obj)
    run(5)

    del ste, fih
    assert h.List("StateTransitionEvent").count() == 0
    assert h.List("FInitializeHandler").count() == 0


if __name__ == "__main__":
    test_ste()

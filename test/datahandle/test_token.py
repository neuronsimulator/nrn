from neuron import h

# model - membrane action potential
def model():
    s = h.Section()
    s.L = s.diam = h.sqrt(100.0 / h.PI)
    s.insert("hh")
    ic = h.IClamp(s(0.5))
    ic.delay = 0.5
    ic.dur = 0.1
    ic.amp = 0.3
    return s, ic


def run(method):
    h.finitialize(-65.0)
    while h.t < 1.0:
        method()


def test_run():
    s, ic = model()
    tvec = h.Vector().record(h._ref_t, sec=s)
    vvec = h.Vector().record(s(0.5)._ref_v, sec=s)

    run(h.fadvance)
    tstd = tvec.c()
    vstd = vvec.c()

    run(h.opaque_advance)
    assert tvec.eq(tstd)
    assert vvec.eq(vstd)


if __name__ == "__main__":
    test_run()

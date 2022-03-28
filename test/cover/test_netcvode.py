from neuron import h

cv = h.CVode()
pc = h.ParallelContext()

# call netcvode.cpp: void nrn_use_daspk(int b)
def nrn_use_daspk():
    cv.use_daspk(0)
    s = h.Section()
    cv.active(1)
    s.insert("extracellular")
    h.finitialize(-65)
    s.diam = 200
    h.finitialize(-65)
    assert cv.use_daspk() == 1.0
    del s
    cv.use_daspk(0)
    cv.active(0)


# call netcvode: static Node*(Object*)
def node():
    # Requires source POINT_PROCESS with "x" threshold variable and lvardt
    # Such a POINT_PROCESS is discouraged in favor of NET_RECEIVE with WATCH.
    s = h.Section()
    s.L = 10
    s.diam = 10
    cv.active(1)
    ic = h.IClamp(s(0.5))
    ic.delay = 0
    ic.dur = 5
    ic.amp = 0.3
    src = h.PPxSrc(s(0.5))
    nc = h.NetCon(src, None)

    results = [
        [
            (0.6500000000999997, 12.070427805842268),
            (0.7000315963318277, 16.84809332604498),
            (0.7000315963318277, 16.84809332604498),
        ],
        [
            (0.6500000000999997, 12.070427805842268),
            (0.6283185307179584, 10.0),
            (0.6283185307179584, 10.0),
        ],
    ]

    def ev(*arg):
        print("ev t=%g v=%g x=%g nc.x=%g" % (h.t, s(0.5).v, src.x, nc.x))
        assert (h.t, src.x) == results[arg[0]][arg[1]]

    def run():
        order = 0
        h.dt = 0.025
        type = (
            "fixed" if cv.active() == 0 else "lvardt" if cv.use_local_dt() else "cvode"
        )
        method = 0 if cv.active() == 0 else 2 if cv.use_local_dt() else 1
        if cv.active():
            type += " condition_order %d" % cv.condition_order()
            order = 0 if cv.condition_order() == 1 else 1
        nc.record((ev, (order, method)))
        print("\n" + type, "  thresh ", nc.threshold)
        pc.set_maxstep(10)
        h.finitialize(-50)
        pc.psolve(1)

    def series(condorder):
        cv.condition_order(condorder)
        cv.active(0)
        run()
        cv.active(1)
        run()
        cv.use_local_dt(1)
        run()
        cv.use_local_dt(0)
        cv.active(0)

    # otherwise, the first order threshold crossing is missed
    cv.maxstep(0.1)
    series(1)
    cv.maxstep(1000)
    series(2)


def test_netcvode_cover():
    nrn_use_daspk()
    node()


if __name__ == "__main__":
    test_netcvode_cover()

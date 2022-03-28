from neuron import h

cv = h.CVode()
pc = h.ParallelContext()

# call netcvode.cpp: void nrn_use_daspk(int b)
def nrn_use_daspk():
    s = h.Section()
    cv.active(1)
    s.insert("extracellular")
    h.finitialize(-65)
    s.diam = 200
    h.finitialize(-65)


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

    def ev():
        print("ev t=%g v=%g x=%g nc.x=%g" % (h.t, s(0.5).v, src.x, nc.x))

    nc.record(ev)

    def run():
        h.dt = 0.025
        type = (
            "fixed" if cv.active() == 0 else "lvardt" if cv.use_local_dt() else "cvode"
        )
        if cv.active():
            type += " condition_order %d" % cv.condition_order()
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

    series(1)
    series(2)


def test_netcvode_cover():
    nrn_use_daspk()
    node()


if __name__ == "__main__":
    test_netcvode_cover()

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
    ic.amp = 0.1
    src = h.PPxSrc(s(0.5))
    nc = h.NetCon(src, None)

    def ev():
        print("ev t=%g v=%g x=%g" % (h.t, s.v(0.5), src.x))

    nc.record(ev)

    def run():
        pc.set_maxstep(10)
        h.finitialize(-50)
        pc.psolve(1)

    run()
    cv.use_local_dt(1)
    run()


def test_netcvode_cover():
    nrn_use_daspk()
    node()


if __name__ == "__main__":
    test_netcvode_cover()

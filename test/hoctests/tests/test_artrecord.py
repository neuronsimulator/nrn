from neuron import h, gui


def test_artrecord():
    ra = h.RanArt()

    erec = h.Vector().record(ra, ra._ref_x2)
    trec = h.Vector().record(ra, h._ref_t)

    def run(tstop):
        h.tstop = tstop
        h.run()
        print("trec")
        trec.printf()
        print("erec")
        erec.printf()

    run(0.6)
    s = h.Section()  # or cvode won't do anything
    h.cvode_active(1)
    run(0.6)

    nc = h.NetCon(ra, None)
    ncvec = h.Vector()
    nc.record(ncvec)
    run(0.6)
    print("ncvec")
    ncvec.printf()


if __name__ == "__main__":
    test_artrecord()

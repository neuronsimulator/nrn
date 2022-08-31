from neuron import h

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
        for x in rv:
            assert a(x).v == rv[x][0]

    cmp(10)

    a.nseg *= 3  # simulations now 9 times more accurate spatially
    cmp(20)

    a.nseg = 5
    cmp(30)


if __name__ == "__main__":
    test_1()

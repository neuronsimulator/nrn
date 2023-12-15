from neuron import h, coreneuron

pc = h.ParallelContext()


def model():
    nslist = [h.NetStim() for _ in range(3)]
    for gid, ns in enumerate(nslist):
        ns.start = 0
        ns.number = 1e9
        ns.interval = 1
        ns.noise = 1
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(ns, None))
    spiketime = h.Vector()
    spikegid = h.Vector()
    pc.spike_record(-1, spiketime, spikegid)
    return nslist, spiketime, spikegid


def pspike(m):
    print("spike raster")
    for i, t in enumerate(m[1]):
        print("%.5f %g" % (t, m[2][i]))


def run(tstop, m):
    pc.set_maxstep(10)
    h.finitialize(-65)
    pc.psolve(tstop)
    pspike(m)


def test1():
    m = model()
    run(5, m)
    coreneuron.enable = True
    coreneuron.verbose = 0
    run(5, m)


if __name__ == "__main__":
    test1()

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


class Cell:
    def __init__(self, id):
        self.id = id
        self.soma = h.Section(name="soma", cell=self)
        self.soma.diam = 10.0
        self.soma.L = 10.0
        self.soma.insert("hh")

    def __str__(self):
        return "Cell" + str(self.id)


class Net:
    # all to all
    def __init__(self, ncell):
        cells = [Cell(i) for i in range(ncell)]
        syns = {}  # {(i,j):(ExpSyn,NetCon)}
        for i in range(ncell):
            for j in range(ncell):
                syn = h.ExpSyn(cells[j].soma(0.5))
                # spikes via inhibitory rebound and synchronizes
                syn.e = -120
                syn.tau = 3
                nc = h.NetCon(cells[i].soma(0.5)._ref_v, syn, sec=cells[i].soma)
                nc.delay = 5
                nc.weight[0] = 0.001
                nc.threshold = -10
                syns[(i, j)] = (syn, nc)
        ic = h.IClamp(cells[0].soma(0.5))
        ic.delay = 0
        ic.dur = 0.1
        ic.amp = 0.3
        self.cells = cells
        self.syns = syns
        self.ic = ic


def netcon_access():
    net = Net(4)

    # NetCon.preloc and NetCon.preseg
    x = net.syns[(0, 1)][1].preloc()
    sec = h.cas()
    h.pop_section()
    assert sec(x) == net.cells[0].soma(0.5)
    assert net.syns[(0, 1)][1].preseg() == net.cells[0].soma(0.5)
    # two more lines of coverage if source is at 0 loc of section
    nc = h.NetCon(
        net.cells[0].soma(0)._ref_v, net.syns[(0, 1)][0], sec=net.cells[0].soma
    )
    x = nc.preloc()
    sec = h.cas()
    h.pop_section()
    assert sec(x) == net.cells[0].soma(0)
    assert nc.preseg() == net.cells[0].soma(0)
    del nc
    # if NetCon source not a section then return -1 or None
    acell = h.NetStim()
    nc = h.NetCon(acell, net.syns[(0, 1)][0])
    x = nc.preloc()
    assert x == -1.0  # and no need to pop the section
    assert nc.preseg() == None
    del nc, acell
    # if NetCon source not a voltage but in a section then return -2 or None
    nc = h.NetCon(
        net.cells[0].soma(0.5).hh._ref_m, net.syns[(0, 1)][0], sec=net.cells[0].soma
    )
    x = nc.preloc()
    assert x == -2.0
    h.pop_section()
    assert nc.preseg() == None
    del nc


def test_netcvode_cover():
    nrn_use_daspk()
    node()
    netcon_access()


if __name__ == "__main__":
    test_netcvode_cover()
    pass

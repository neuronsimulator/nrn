# test nmodl LAG statement.
# 5 cells with distinct trajectories and distinct lags
# Show trajectories in Graph and compare shifted trajectories

from neuron import h, gui

pc = h.ParallelContext()


class Cell:
    def __init__(self, id):
        self.id = id
        s = h.Section(name="soma", cell=self)
        self.soma = s
        s.L = 10
        s.diam = 10
        s.insert("hh")
        self.lag = h.LagV(s(0.5))
        self.istim = h.IClamp(s(0.5))
        self.istim.dur = 0.1
        self.istim.amp = 0.3

    def __str__(self):
        return "Cell_" + str(self.id)


cells = [Cell(i) for i in range(5)]
trajectories = []
for i, cell in enumerate(cells):
    cell.soma.gkbar_hh = 0.036 + 0.003 * i  # distinct AP trajectories
    cell.lag.dur = 1.0 + i
    trajectories.append(
        (h.Vector().record(cell.soma(0.5)._ref_v), h.Vector().record(cell.lag._ref_vl))
    )

tvec = h.Vector()
tvec.record(h._ref_t, sec=cells[0].soma)


def run(tstop):
    pc.set_maxstep(10)
    h.finitialize(-65.0)
    pc.psolve(tstop)


run(10)
g = h.Graph()
for trajec in trajectories:
    for v in trajec:
        v.line(g, tvec)
g.exec_menu("View = plot")


def chk(cells, trajectories):
    for i, cell in enumerate(cells):
        trajec = trajectories[i]
        start = cell.lag.dur / h.dt
        end = 4 / h.dt
        err = trajec[0].c(0, end).sub(trajec[1].c(start, start + end)).abs().sum()
        assert err < 1e-9


chk(cells, trajectories)

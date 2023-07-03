from neuron import h, crxd as rxd, gui
from neuron.crxd import v
from neuron.crxd.rxdmath import exp, log, vtrap
from math import pi

h.load_file("stdrun.hoc")

# parameters
h.celsius = 6.3
e = 1.60217662e-19
scale = 1e-14 / e
gnabar = 0.12 * scale  # molecules/um2 ms mV
gkbar = 0.036 * scale
gl = 0.0003 * scale
el = -54.3
q10 = 3.0 ** ((h.celsius - 6.3) / 10.0)

# sodium activation 'm'
alpha = 0.1 * vtrap(-(v + 40.0), 10)
beta = 4.0 * exp(-(v + 65) / 18.0)
mtau = 1.0 / (q10 * (alpha + beta))
minf = alpha / (alpha + beta)


# sodium inactivation 'h'
alpha = 0.07 * exp(-(v + 65.0) / 20.0)
beta = 1.0 / (exp(-(v + 35.0) / 10.0) + 1.0)
htau = 1.0 / (q10 * (alpha + beta))
hinf = alpha / (alpha + beta)

# potassium activation 'n'
alpha = 0.01 * vtrap(-(v + 55.0), 10.0)
beta = 0.125 * exp(-(v + 65.0) / 80.0)
ntau = 1.0 / (q10 * (alpha + beta))
ninf = alpha / (alpha + beta)

h.load_file("import3d.hoc")
cell = h.Import3d_SWC_read()
cell.input("c91662.swc")
i3d = h.Import3d_GUI(cell, 0)


class Cell:
    def move(self, offset=(0, 0, 0)):
        ox, oy, oz = offset[0], offset[1], offset[2]
        h.define_shape()
        xlo = ylo = zlo = xhi = yhi = zhi = None
        for sec in self.all:
            sec.nseg = 1
            n3d = sec.n3d()
            xs = [sec.x3d(i) + ox for i in range(n3d)]
            ys = [sec.y3d(i) + oy for i in range(n3d)]
            zs = [sec.z3d(i) + oz for i in range(n3d)]
            ds = [sec.diam3d(i) for i in range(n3d)]
            sec.pt3dclear()
            for x, y, z, d in zip(xs, ys, zs, ds):
                sec.pt3dadd(x, y, z, d)
            my_xlo, my_ylo, my_zlo = min(xs), min(ys), min(zs)
            my_xhi, my_yhi, my_zhi = max(xs), max(ys), max(zs)
            if xlo is None:
                xlo, ylo, zlo = my_xlo, my_ylo, my_zlo
                xhi, yhi, zhi = my_xhi, my_yhi, my_zhi
            else:
                xlo, ylo, zlo = min(xlo, my_xlo), min(ylo, my_ylo), min(zlo, my_zlo)
                xhi, yhi, zhi = max(xhi, my_xhi), max(yhi, my_yhi), max(zhi, my_zhi)
        return xlo, ylo, zlo, xhi, yhi, zhi


mycellA = Cell()
mycellB = Cell()
i3d.instantiate(mycellA)
i3d.instantiate(mycellB)

xloA, yloA, zloA, xhiA, yhiA, zhiA = mycellA.move((-100, 0, 0))
xloB, yloB, zloB, xhiB, yhiB, zhiB = mycellB.move((100, 0, 0))
xl, yl, zl = min(xloA, xloB) - 50, min(yloA, yloB) - 50, min(zloA, zloB) - 50
xh, yh, zh = max(xhiA, xhiB) + 50, max(yhiA, yhiB) + 50, max(zhiA, zhiB) + 50

# mod version
for sec in mycellB.all:
    sec.insert("hh")

# rxd version

# Where?
# intracellular
cyt = rxd.Region(h.allsec(), name="cyt", nrn_region="i")

# membrane
mem = rxd.Region(h.allsec(), name="cell_mem", geometry=rxd.membrane())

# extracellular
ecs = rxd.Extracellular(xl, yl, zl, xh, yh, zh, dx=50)

# Who?


def init(ics, ecs):
    return lambda nd: ecs if isinstance(nd, rxd.node.NodeExtracellular) else ics


# ions
k = rxd.Species([cyt, mem, ecs], name="k", d=1, charge=1, initial=init(54.4, 2.5))

na = rxd.Species([cyt, mem, ecs], name="na", d=1, charge=1, initial=init(10.0, 140.0))
x = rxd.Species([cyt, mem, ecs], name="x", charge=1)

ki, ko, nai, nao, xi, xo = k[cyt], k[ecs], na[cyt], na[ecs], x[cyt], x[ecs]

# gates
ngate = rxd.Species([cyt, mem], name="ngate", initial=0.24458654944007166)
mgate = rxd.Species([cyt, mem], name="mgate", initial=0.028905534475191907)
hgate = rxd.Species([cyt, mem], name="hgate", initial=0.7540796658225246)

# mycellA parameter
pA = rxd.Parameter(
    [cyt, mem],
    name="paramA",
    initial=lambda nd: 1 if nd.segment.sec in mycellA.all else 0,
)

# What
# gates
m_gate = rxd.Rate(mgate, (minf - mgate) / mtau)
h_gate = rxd.Rate(hgate, (hinf - hgate) / htau)
n_gate = rxd.Rate(ngate, (ninf - ngate) / ntau)

# Nernst potentials
ena = 1e3 * h.R * (h.celsius + 273.15) * log(nao / nai) / h.FARADAY  # 63
ek = 1e3 * h.R * (h.celsius + 273.15) * log(ko / ki) / h.FARADAY  # -74

gna = pA * gnabar * mgate**3 * hgate
gk = pA * gkbar * ngate**4

na_current = rxd.MultiCompartmentReaction(
    nai, nao, gna * (v - ena), mass_action=False, membrane=mem, membrane_flux=True
)
k_current = rxd.MultiCompartmentReaction(
    ki, ko, gk * (v - ek), mass_action=False, membrane=mem, membrane_flux=True
)
leak_current = rxd.MultiCompartmentReaction(
    xi, xo, pA * gl * (v - el), mass_action=False, membrane=mem, membrane_flux=True
)

# stimulate
stimA = h.IClamp(mycellA.soma[0](0.5))
stimA.delay = 50
stimA.amp = 5
stimA.dur = 50

stimB = h.IClamp(mycellB.soma[0](0.5))
stimB.delay = 50
stimB.amp = 5
stimB.dur = 50

# record
tvec = h.Vector().record(h._ref_t)

vvecA = h.Vector().record(mycellA.soma[0](0.5)._ref_v)
mvecA = h.Vector().record(mgate.nodes(mycellA.soma[0](0.5))[0]._ref_value)
nvecA = h.Vector().record(ngate.nodes(mycellA.soma[0](0.5))[0]._ref_value)
hvecA = h.Vector().record(hgate.nodes(mycellA.soma[0](0.5))[0]._ref_value)
kvecA = h.Vector().record(mycellA.soma[0](0.5)._ref_ik)
navecA = h.Vector().record(mycellA.soma[0](0.5)._ref_ina)

vvecB = h.Vector().record(mycellB.soma[0](0.5)._ref_v)
kvecB = h.Vector().record(mycellB.soma[0](0.5)._ref_ik)
navecB = h.Vector().record(mycellB.soma[0](0.5)._ref_ina)
mvecB = h.Vector().record(mycellB.soma[0](0.5).hh._ref_m)
nvecB = h.Vector().record(mycellB.soma[0](0.5).hh._ref_n)
hvecB = h.Vector().record(mycellB.soma[0](0.5).hh._ref_h)
tvec = h.Vector().record(h._ref_t)

# run
h.dt = 0.01
h.finitialize(-70)
# for i in range(1000):
#    h.fadvance()
#    print(h.t,mycellA.soma[0](0.5).v, mycellA.soma[0](0.5).ki, mycellA.soma[0](0.5).nai, mycellA.soma[0](0.5).xi)

h.continuerun(100)


if __name__ == "__main__":
    from matplotlib import pyplot

    # plot the results
    pyplot.ion()
    fig = pyplot.figure()
    pyplot.plot(tvec, vvecA, label="rxd")
    pyplot.plot(tvec, vvecB, label="mod")
    pyplot.legend()
    fig.set_dpi(200)

    fig = pyplot.figure()
    pyplot.plot(tvec, hvecA, "-b", label="h")
    pyplot.plot(tvec, mvecA, "-r", label="m")
    pyplot.plot(tvec, nvecA, "-g", label="n")
    pyplot.plot(tvec, hvecB, ":b")
    pyplot.plot(tvec, mvecB, ":r")
    pyplot.plot(tvec, nvecB, ":g")
    pyplot.legend()
    fig.set_dpi(200)

    fig = pyplot.figure()
    pyplot.plot(tvec, kvecA, "-b", label="k")
    pyplot.plot(tvec, navecA, "-r", label="na")
    pyplot.plot(tvec, kvecB, ":b")
    pyplot.plot(tvec, navecB, ":r")
    pyplot.legend()
    fig.set_dpi(200)

    fig = pyplot.figure()
    pyplot.plot(tvec, kvecA.as_numpy(), "-b", label="k")
    pyplot.plot(tvec, navecA.as_numpy(), "-r", label="na")
    pyplot.plot(tvec, kvecB, ":b")
    pyplot.plot(tvec, navecB, ":r")
    pyplot.legend()
    fig.set_dpi(200)

    fig = pyplot.figure()
    pyplot.plot(tvec, kvecA.as_numpy() - kvecB, "-b", label="k")
    pyplot.plot(tvec, navecA.as_numpy() - navecB, "-r", label="na")
    pyplot.legend()
    fig.set_dpi(200)

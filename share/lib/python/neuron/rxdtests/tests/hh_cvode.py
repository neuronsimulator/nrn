from neuron import h, crxd as rxd
from neuron.crxd import v
from neuron.crxd.rxdmath import vtrap, exp, log
from math import pi

h.load_file("stdrun.hoc")
h.CVode().active(True)
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

somaA = h.Section("somaA")
somaA.pt3dclear()
somaA.pt3dadd(-90, 0, 0, 1)
somaA.pt3dadd(-60, 0, 0, 1)
somaA.nseg = 11

somaB = h.Section("somaB")
somaB.pt3dclear()
somaB.pt3dadd(60, 0, 0, 1)
somaB.pt3dadd(90, 0, 0, 1)
somaB.nseg = 11

# mod version
somaB.insert("hh")

# rxd version

# Where?
# intracellular
cyt = rxd.Region(h.allsec(), name="cyt", nrn_region="i")

# membrane
mem = rxd.Region(h.allsec(), name="cell_mem", geometry=rxd.membrane())

# extracellular
ecs = rxd.Extracellular(-100, -100, -100, 100, 100, 100, dx=100)


# Who?
def init(ics, ecs):
    return lambda nd: ecs if isinstance(nd, rxd.node.NodeExtracellular) else ics


# ions
k = rxd.Species(
    [cyt, mem, ecs],
    name="k",
    d=1e3,
    charge=1,
    initial=init(54.4, 2.5),
    ecs_boundary_conditions=2.5,
)

na = rxd.Species(
    [cyt, mem, ecs],
    name="na",
    d=1e3,
    charge=1,
    initial=init(10.0, 140.0),
    ecs_boundary_conditions=140,
)
x = rxd.Species([cyt, mem, ecs], name="x", charge=1)

ki, ko, nai, nao, xi, xo = k[cyt], k[ecs], na[cyt], na[ecs], x[cyt], x[ecs]

# gates
ngate = rxd.State([cyt, mem], name="ngate", initial=0.24458654944007166)
mgate = rxd.State([cyt, mem], name="mgate", initial=0.028905534475191907)
hgate = rxd.State([cyt, mem], name="hgate", initial=0.7540796658225246)

# somaA parameter
pA = rxd.Parameter(
    [cyt, mem], name="paramA", initial=lambda nd: 1 if nd.segment in somaA else 0
)

# What
# gates
m_gate = rxd.Rate(mgate, (minf - mgate) / mtau)
h_gate = rxd.Rate(hgate, (hinf - hgate) / htau)
n_gate = rxd.Rate(ngate, (ninf - ngate) / ntau)

# Nernst potentials
ena = 1e3 * h.R * (h.celsius + 273.15) * log(nao / nai) / h.FARADAY
ek = 1e3 * h.R * (h.celsius + 273.15) * log(ko / ki) / h.FARADAY

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
stimA = h.IClamp(somaA(0.5))
stimA.delay = 50
stimA.amp = 1
stimA.dur = 50

stimB = h.IClamp(somaB(0.5))
stimB.delay = 50
stimB.amp = 1
stimB.dur = 50

# record
tvec = h.Vector().record(h._ref_t)

vvecA = h.Vector().record(somaA(0.5)._ref_v)
mvecA = h.Vector().record(mgate.nodes(somaA(0.5))[0]._ref_value)
nvecA = h.Vector().record(ngate.nodes(somaA(0.5))[0]._ref_value)
hvecA = h.Vector().record(hgate.nodes(somaA(0.5))[0]._ref_value)
kvecA = h.Vector().record(somaA(0.5)._ref_ik)
navecA = h.Vector().record(somaA(0.5)._ref_ina)

vvecB = h.Vector().record(somaB(0.5)._ref_v)
kvecB = h.Vector().record(somaB(0.5)._ref_ik)
navecB = h.Vector().record(somaB(0.5)._ref_ina)
mvecB = h.Vector().record(somaB(0.5).hh._ref_m)
nvecB = h.Vector().record(somaB(0.5).hh._ref_n)
hvecB = h.Vector().record(somaB(0.5).hh._ref_h)
tvec = h.Vector().record(h._ref_t)

# run
h.dt = 0.025
h.finitialize(-70)
# for i in range(1000):
#    h.fadvance()
#    print(h.t,somaA(0.5).v, somaA(0.5).ki, somaA(0.5).nai, somaA(0.5).xi)
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

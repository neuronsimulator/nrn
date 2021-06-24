from neuron import h, crxd as rxd, gui
import numpy
import sys
import time
import itertools

npar = len(sys.argv)
# if(npar<2):
#    print "usage: python wave1d.py <nseg> <nsubseg>"
#    sys.exit(0)

# rxd.options.nsubseg =int(sys.argv[2])
rxd.options.subseg_interpolation = 0
rxd.options.subseg_averaging = 0


sec = h.Section()
L = 100
# sec.diam = 1
sec.nseg = 25
# h.pt3dadd(0, 0, 0, 1)
# h.pt3dadd(L, 0, 0, 10)
sec.Ra = 150
Rm = 25370
dend = sec
for myseg in dend:
    myseg.v = -64
for myseg in dend:
    myseg.cm = 1.41
dend.insert("pas")
for myseg in dend:
    myseg.pas.g = 1.0 / Rm
for myseg in dend:
    myseg.pas.e = -64
dend.insert("cal")  # insert L-type Ca channel
for myseg in dend:
    myseg.cal.gcalbar = 1.0e-6
h.pt3dadd(0, 0, 0, 1)
h.pt3dadd(L, 0, 0, 10)

nsubseg = 5


nstim = 5
st_dur = 2
st_interv = 50
st_start = 1000
stims = []
for i in range(nstim):
    stim = h.IClamp(0.5, sec=sec)
    stim.delay = st_start + i * (st_interv)
    stim.dur = st_dur
    stim.amp = 1.0 / 8000 * 1200
    stims.append(stim)

h.CVode().active(1)
h.cvode.atol(1e-8)

s = h.PlotShape()

s.exec_menu("Shape Plot")
if __name__ == "__main__":

    def set_plotshape_colormap(plotshape, cmap="jet"):
        import matplotlib.cm

        s = matplotlib.cm.ScalarMappable(cmap=cmap)
        cmap = s.get_cmap()
        s.set_clim(0, cmap.N)
        rs, gs, bs = itertools.islice(zip(*s.to_rgba(list(range(cmap.N)))), 0, 3)
        plotshape.colormap(cmap.N)
        for i, r, g, b in zip(range(cmap.N), rs, gs, bs):
            plotshape.colormap(i, r * 255, g * 255, b * 255)
        # call s.scale(lo, hi) to replot the legend

    set_plotshape_colormap(s)


# show the diameters
s.show(0)

# cytoplasmic, er volume fractions
fc, fe = 0.83, 0.17

# parameters
caDiff = 0.016
# caDiff =0
ip3Diff = 0.283
# ip3Diff = 0
cac_init = 1.0e-4
ip3_init = 0.1
gip3r = 12040
gserca = 0.3913
gleak = 6.020
kserca = 0.1
kip3 = 0.15
kact = 0.4
ip3rtau = 2000

# define the regions for the rxd
cyt = rxd.Region(
    h.allsec(), nrn_region="i", geometry=rxd.FractionalVolume(fc, surface_fraction=1)
)
er = rxd.Region(h.allsec(), geometry=rxd.FractionalVolume(fe))
cyt_er_membrane = rxd.Region(h.allsec(), geometry=rxd.FixedPerimeter(1))

# the species and other states
ca = rxd.Species([cyt, er], d=caDiff, name="ca", charge=2, initial=cac_init)
ip3 = rxd.Species(cyt, d=ip3Diff, initial=ip3_init)
ip3r_gate_state = rxd.State(cyt_er_membrane, initial=0.8)
h_gate = ip3r_gate_state[cyt_er_membrane]


# pumps and channels between ER and Cytosol

serca = rxd.MultiCompartmentReaction(
    ca[cyt] > ca[er],
    gserca / ((kserca / (1000.0 * ca[cyt])) ** 2 + 1),
    membrane=cyt_er_membrane,
    custom_dynamics=True,
)
leak = rxd.MultiCompartmentReaction(
    ca[er] != ca[cyt], gleak, gleak, membrane=cyt_er_membrane
)

minf = ip3[cyt] * 1000.0 * ca[cyt] / (ip3[cyt] + kip3) / (1000.0 * ca[cyt] + kact)
k = gip3r * (minf * h_gate) ** 3
ip3r = rxd.MultiCompartmentReaction(ca[er] != ca[cyt], k, k, membrane=cyt_er_membrane)
ip3rg = rxd.Rate(h_gate, (1.0 / (1 + 1000.0 * ca[cyt] / (0.3)) - h_gate) / ip3rtau)

v1 = h.Vector()
v1.record(sec(0.5)._ref_v)
ca1 = h.Vector()
ca1.record(sec(0.5)._ref_cai)
v2 = h.Vector()
v2.record(sec(0.25)._ref_v)
ca2 = h.Vector()
ca2.record(sec(0.25)._ref_cai)
times = h.Vector()
times.record(h._ref_t)


h.finitialize()

cae_init = (0.0017 - cac_init * fc) / fe
ca[er].concentration = cae_init

# ip3.nodes.concentration = 2
for node in ip3.nodes:
    if node.x < 0.2:
        node.concentration = 2


h.CVode().re_init()

s.variable("cai")
# s.scale(-70, -50)
s.scale(0, 2e-3)


tstop = 3000
recdt = 100
datacol = 0
del s
h.continuerun(tstop)

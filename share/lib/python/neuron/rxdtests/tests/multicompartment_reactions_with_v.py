# Example copied from the RxD tutorial
# https://nrn.readthedocs.io/en/latest/rxd-tutorials/index.html
from neuron import crxd as rxd, h, gui
from neuron.crxd import v
import numpy

sec = h.Section()
sec.L = 100
sec.diam = 1
sec.nseg = 100

h.CVode().active(1)
h.CVode().atol(1e-4)


caDiff = 0.016
ip3Diff = 0.283
cac_init = 1.0e-4
ip3_init = 0.1
gip3r = 12040
gserca = 0.3913
gleak = 6.020
kserca = 0.1
kip3 = 0.15
kact = 0.4
ip3rtau = 2000.0

rescale_v = 1000 / -65.0

# These parameters where missing in the tutorial so arbitrary values were chosen
# any resemblance to experimental values is purely coincidental.
fc = 0.7
fe = 0.3
caCYT_init = 0.1


cyt = rxd.Region(
    h.allsec(),
    name="cyt",
    nrn_region="i",
    geometry=rxd.FractionalVolume(fc, surface_fraction=1),
)

er = rxd.Region(h.allsec(), name="er", geometry=rxd.FractionalVolume(fe / 2.0))

cyt_er_membrane = rxd.Region(
    h.allsec(), name="mem", geometry=rxd.ScalableBorder(1, on_cell_surface=False)
)

ca = rxd.Species([cyt, er], d=caDiff, name="ca", charge=2, initial=caCYT_init)


ip3 = rxd.Species(cyt, d=ip3Diff, name="ip3", initial=ip3_init)
ip3r_gate_state = rxd.Species(cyt_er_membrane, name="gate", initial=0.8)
h_gate = ip3r_gate_state[cyt_er_membrane]
minf = (
    ip3[cyt]
    * rescale_v
    * v
    * ca[cyt]
    / (ip3[cyt] + kip3)
    / (rescale_v * v * ca[cyt] + kact)
)
k = gip3r * (minf * h_gate) ** 3

ip3r = rxd.MultiCompartmentReaction(ca[er], ca[cyt], k, k, membrane=cyt_er_membrane)

serca = rxd.MultiCompartmentReaction(
    ca[cyt],
    ca[er],
    gserca / ((kserca / (1000.0 * ca[cyt])) ** 2 + 1),
    membrane=cyt_er_membrane,
    custom_dynamics=True,
)
leak = rxd.MultiCompartmentReaction(
    ca[er], ca[cyt], gleak, gleak, membrane=cyt_er_membrane
)


ip3r = rxd.MultiCompartmentReaction(ca[er], ca[cyt], k, k, membrane=cyt_er_membrane)
ip3rg = rxd.Rate(
    h_gate, (1.0 / (1 + rescale_v * v * ca[cyt] / (0.3)) - h_gate) / ip3rtau
)

h.finitialize()

cacyt_trace = h.Vector()
cacyt_trace.record(ca[cyt].nodes(sec)(0.5)[0]._ref_concentration)

caer_trace = h.Vector()
caer_trace.record(ca[er].nodes(sec)(0.5)[0]._ref_concentration)

ip3_trace = h.Vector()
ip3_trace.record(ip3.nodes(sec)(0.5)[0]._ref_concentration)

times = h.Vector()
times.record(h._ref_t)

h.finitialize()

cae_init = (0.0017 - cac_init * fc) / fe
ca[er].concentration = cae_init

for node in ip3.nodes:
    if node.x < 0.2:
        node.concentration = 2


h.CVode().re_init()
h.continuerun(1000)

if __name__ == "__main__":
    from matplotlib import pyplot

    pyplot.plot(times, cacyt_trace, label="ca[cyt]")
    pyplot.plot(times, caer_trace, label="ca[er]")
    pyplot.plot(times, ip3_trace, label="ip3")
    pyplot.legend()
    pyplot.show()

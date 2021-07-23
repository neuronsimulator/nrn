from neuron import h, rxd

h.load_file("stdrun.hoc")
h.CVode().active(True)

sec = h.Section(name="sec")
sec.L = 10
sec.nseg = 11
sec.diam = 5
rxd.set_solve_type(dimension=3)

cyt = rxd.Region(h.allsec(), name="cyt", nrn_region="i")
ip3 = rxd.Species(
    cyt, name="ip3", initial=lambda nd: 1000 if nd.segment == sec(0.3) else 0
)


def callbackfun():
    return 1000


for nd in ip3.nodes(sec(0.1)):
    nd.include_flux(1000)

for nd in ip3.nodes(sec(0.5)):
    nd.include_flux(callbackfun)

for nd in ip3.nodes(sec(0.9)):
    nd.include_flux(sec(0.3)._ref_ip3i)

h.finitialize(-70)
h.CVode().event(10)
h.continuerun(10)
import numpy

ip33d = ip3._intracellular_instances[cyt]
con = []
for seg in sec:
    con.append(
        numpy.mean([nd.concentration * nd.volume for nd in ip3.nodes(seg)])
        / seg.volume()
    )
print(con)

con1D = [
    0.0,
    0.0009302760807252256,
    0.0,
    1000.0,
    0.0,
    0.0009302760807252256,
    0.0,
    0.0,
    0.0,
    0.000930276080725227,
    0.0,
]

diff = numpy.array(con) - numpy.array(con1D)

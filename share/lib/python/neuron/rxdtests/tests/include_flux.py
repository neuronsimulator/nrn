from neuron import h, rxd

h.load_file("stdrun.hoc")

sec = h.Section(name="sec")
sec.L = 10
sec.nseg = 11
sec.diam = 5

cyt = rxd.Region(h.allsec(), name="cyt", nrn_region="i")
ip3 = rxd.Species(
    cyt, name="ip3", initial=lambda nd: 1000 if nd.segment == sec(0.3) else 0
)


def callbackfun():
    return 1000


h.finitialize(-70)

for nd in ip3.nodes(sec(0.1)):
    nd.include_flux(1000)

for nd in ip3.nodes(sec(0.5)):
    nd.include_flux(callbackfun)

for nd in ip3.nodes(sec(0.9)):
    nd.include_flux(sec(0.3)._ref_ip3i)

h.continuerun(10)
print(ip3.nodes.concentration)

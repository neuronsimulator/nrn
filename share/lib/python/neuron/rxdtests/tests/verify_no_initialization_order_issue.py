from neuron import h, crxd as rxd, gui
from neuron.crxd import initializer

h.CVode().active(1)

rxdsec = [h.Section(), h.Section()]
cyt = rxd.Region(rxdsec)
er = rxd.Region(rxdsec)
ca = rxd.Species([cyt, er])
caextrude = rxd.Rate(ca, 1)

# test for forced initialization before model fully described
initializer._do_init()
ip3 = rxd.Species(cyt)
h.tstop = 1000
h.run()

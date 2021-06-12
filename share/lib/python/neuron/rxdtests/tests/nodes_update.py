from neuron import h, crxd as rxd

h.load_file("stdrun.hoc")
soma = h.Section(name="soma")
r = rxd.Region([soma])
soma.nseg = 5
c = rxd.Species(r, initial=1.0)
assert len(c.nodes) == 5
soma.nseg = 3
assert len(c.nodes) == 3
soma.nseg = 7
assert len(c[r].nodes) == 7
h.tstop = 100
h.run()

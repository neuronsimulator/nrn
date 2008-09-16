from neuron import h

pc = h.ParallelContext()

print "NEURON thinks I am %d of %d\n" % (pc.id(),pc.nhost())

pc.done()

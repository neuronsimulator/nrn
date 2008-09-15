from mpi4py import MPI
from neuron import h

pc = h.ParallelContext()

print "I am %d of %d\n" % (pc.id(),pc.nhost())

pc.done()

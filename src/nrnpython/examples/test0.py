from mpi4py import MPI
from neuron import h

pc = h.ParallelContext()

print "mpi4py thinks I am %d of %d, NEURON thinks I am %d of %d\n" % (MPI.COMM_WORLD.rank, MPI.COMM_WORLD.size, pc.id(),pc.nhost())

pc.done()

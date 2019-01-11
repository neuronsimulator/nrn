from neuron import h
pc = h.ParallelContext()
pc.mpi_init()

id = int(pc.id())
nhost = int(pc.nhost())

print ('I am %d of %d'%(id, nhost))

pc.barrier()
h.quit()

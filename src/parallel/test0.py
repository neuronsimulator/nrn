from neuron import h
h.nrnmpi_init()
pc = h.ParallelContext()

id = int(pc.id())
nhost = int(pc.nhost())

print ('I am %d of %d'%(id, nhost))

pc.barrier()
h.quit()

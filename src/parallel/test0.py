from neuron import h
pc = h.ParallelContext()

id = int(pc.id())
nhost = int(pc.nhost())

print ('I am %d of %d'%(id, nhost))

pc.runworker()
pc.done()
h.quit()

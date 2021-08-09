from neuron import h

h.nrnmpi_init()  # initialize MPI
pc = h.ParallelContext()
print("I am {} of {}".format(pc.id(), pc.nhost()))
h.quit()

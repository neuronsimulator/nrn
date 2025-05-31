from neuron import h

pc = h.ParallelContext()

soma = h.Section(name="soma")
soma.nseg = 5
soma.insert("hh")
pv = soma(0.5).hh._ref_m
print(pv)
h.finitialize(-65.0)

pc.nrncore_refvar(pv)

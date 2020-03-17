from neuron import h, gui
import sys

h('''create soma''')
h.soma.L=5.6419
h.soma.diam=5.6419
h.soma.insert("hh")
ic = h.IClamp(h.soma(.5))
ic.delay = .5
ic.dur = 0.1
ic.amp = 0.3

# for testing external mod file
h.soma.insert("CaDynamics_E2")

h.cvode.use_fast_imem(1)
h.cvode.cache_efficient(1)

v = h.Vector()
v.record(h.soma(.5)._ref_v, sec = h.soma)
i_mem = h.Vector()
i_mem.record(h.soma(.5)._ref_i_membrane_, sec = h.soma)
tv = h.Vector()
tv.record(h._ref_t, sec=h.soma)
h.run()
vstd = v.cl()
tvstd = tv.cl()
i_memstd = i_mem.cl()

#h.CoreNeuronRun[0].run()
pc = h.ParallelContext()
h.stdinit()
pc.nrncore_run("-e %g"%h.tstop, 0)

if not bool(tv.eq(tvstd)):
    print("Voltage times are different")
    sys.exit(-1)
if v.cl().sub(vstd).abs().max() >= 1e-10:
    print(v.cl().sub(vstd).abs())
    print("Voltage difference greater than or equal to 1e-10")
    sys.exit(-1)
if i_mem.cl().sub(i_memstd).abs().max() >= 1e-10:
    print("i_mem difference greater than or equal to 1e-10")
    sys.exit(-1)

print("Voltage times and i_membrane_ are same and difference is less than 1e-10")
h.quit()

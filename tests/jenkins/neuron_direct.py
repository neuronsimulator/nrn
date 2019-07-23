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

v = h.Vector()
v.record(h.soma(.5)._ref_v, sec = h.soma)
tv = h.Vector()
tv.record(h._ref_t, sec=h.soma)
h.run()
vstd = v.cl()
tvstd = tv.cl()

#h.CoreNeuronRun[0].run()
pc = h.ParallelContext()
h.cvode.cache_efficient(1)
h.stdinit()
pc.nrncore_run("-e %g"%h.tstop, 0)

if not bool(tv.eq(tvstd)):
    print("Voltage times are different")
    sys.exit(-1)
if v.cl().sub(vstd).abs().max() >= 1e-10:
    print("Voltage difference greater than or equal to 1e-10")
    sys.exit(-1)

print("Voltage times are same and difference is less than 1e-10")
h.quit()

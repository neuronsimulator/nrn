from neuron import h
a = h.Section()
vec = h.Vector(10).indgen().add(100)
tp = []
for i in range(0, 10):
  tp.append(h.t1(.5, sec=a))


a.v = 10

tp[2].f1()

h.setpointer(a(.5)._ref_v, 'p1', tp[0])
for i in range(1, 10):
  h.setpointer(vec._ref_x[i], 'p1', tp[i])

for i in range(0, 10):
  print i, tp[i].p1, tp[i].f1()

tp[2].p1 = 25
print vec[2], tp[2].p1

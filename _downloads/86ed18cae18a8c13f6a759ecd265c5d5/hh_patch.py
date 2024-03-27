from neuron import h, gui

soma = h.Section(name="soma")
soma.insert(h.hh)
soma.L = 5
soma.diam = 5

iclamp = h.IClamp(soma(0.5))
iclamp.delay = 0.1
iclamp.dur = 0.1
iclamp.amp = 0.3

g = h.Graph(False)
g.view(0, -80, 5, 120, 624, 40, 300, 200)
h.graphList[0].append(g)
g.addvar("v", soma(0.5)._ref_v)

# popup the variable step and run controls
h.numericalmethodpanel.map()
h.nrncontrolmenu()

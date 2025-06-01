from neuron import n, gui

soma = n.Section(name="soma")
soma.insert(n.hh)
soma.L = 5
soma.diam = 5

iclamp = n.IClamp(soma(0.5))
iclamp.delay = 0.1
iclamp.dur = 0.1
iclamp.amp = 0.3

g = n.Graph(False)
g.view(0, -80, 5, 120, 624, 40, 300, 200)
n.graphList[0].append(g)
g.addvar("v", soma(0.5)._ref_v)

# popup the variable step and run controls
h.numericalmethodpanel.map()
n.nrncontrolmenu()

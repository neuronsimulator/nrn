from neuron import h, gui

h.load_file(h.neuronhome() + "/demo/pyramid.nrn")
for sec in h.allsec():
    sec.insert("pas")
h.soma.uninsert("pas")

psh = h.PlotShape()
psh.variable("g_pas")
psh.exec_menu("Shape Plot")

# cover a line in ShapeSection::set_range_variable
h.delete_section(sec=h.soma)
psh.variable("g_pas")

import os
from testutils import compare_data, tol

def test_soma_outlines(neuron_instance):
    """Import a Neurolucida model and use the soma outline for voxelization."""

    
    h, rxd, data, save_path = neuron_instance
    
    class Cell:
        def __init__(self):
            self.cell = h.Import3d_Neurolucida3()
            path = os.path.dirname(os.path.abspath(__file__))
            self.cell.input(os.path.join(path,"simple.asc"))
            i3d = h.Import3d_GUI(self.cell, False)
            i3d.instantiate(self)
            for sec in self.all:
                sec.nseg = int(sec.L / 40) * 2 + 1
    c = Cell()
    rxd.set_solve_type(c.all, dimension=3)
    cyt = rxd.Region(c.all, name='cyt', nrn_region='i', dx=1.0)
    ca = rxd.Species(cyt, initial=lambda node: 1 if node in c.soma[0] else 0, d=0.5, charge=2)

    h.finitialize(-65)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol

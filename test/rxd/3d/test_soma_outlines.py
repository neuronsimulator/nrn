import os
import numpy
import pytest
from testutils import compare_data, tol

@pytest.fixture
def cell_model(neuron_instance):
    """Provide a Cell class that use the simple.asc Neurolucida morphology file."""

    h, rxd, data, save_path = neuron_instance

    class Cell:
        """Create a model cell based on the simple.asc Neurolucida file.
    
        Args:
            shift (triplet, optional) displacement in  x, y, z of the cell,
                default (0,0,0).
        """
        def __init__(self, shift=(0,0,0), rotate=0):
            self.cell = h.Import3d_Neurolucida3()
            path = os.path.dirname(os.path.abspath(__file__))
            self.cell.input(os.path.join(path,"simple.asc"))
            i3d = h.Import3d_GUI(self.cell, False)
            i3d.instantiate(self)
            for sec in self.all:
                sec.nseg = int(sec.L / 40) * 2 + 1
            sx, sy, sz = shift
            for sec in self.soma:
                pts = [(sec.x3d(i), sec.y3d(i), sec.z3d(i), sec.diam3d(i))
                        for i in range(sec.n3d())]
                sec.pt3dclear()
                for (x, y, z, diam) in pts:
                    sec.pt3dadd(x + sx, y + sy, z + sz, diam)

    yield (h, rxd, data, save_path, Cell)

def test_soma_outlines(cell_model):
    """Import a Neurolucida model and use the soma outline for voxelization."""

    h, rxd, data, save_path, Cell = cell_model
    c = Cell()
    rxd.set_solve_type(c.all, dimension=3)
    cyt = rxd.Region(c.all, name='cyt', nrn_region='i', dx=1.0)
    ca = rxd.Species(cyt, initial=lambda node: 1 if node in c.soma[0] else 0, d=0.5, charge=2)

    h.finitialize(-65)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_soma_move(cell_model):
    """Import a Neurolucida model and move it."""
    
    h, rxd, data, save_path, Cell = cell_model
    c = Cell(shift=(20,10,20))
    rxd.set_solve_type(c.all, dimension=3)
    cyt = rxd.Region(c.all, name='cyt', nrn_region='i', dx=1.0)
    ca = rxd.Species(cyt, initial=lambda node: 1 if node in c.soma[0] else 0, d=0.5, charge=2)

    h.finitialize(-65)
    h.continuerun(10)
    assert(min([nd.x3d for nd in ca.nodes(c.soma[0])]) > 29)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_multiple_soma(cell_model):
    """Create two Neurolucida models."""

    h, rxd, data, save_path, Cell = cell_model
    c = Cell()
    c2 = Cell(shift=(20,10,20))
    rxd.set_solve_type(c.all, dimension=3)
    cyt = rxd.Region(c.all, name='cyt', nrn_region='i', dx=1.0)
    ca = rxd.Species(cyt, initial=lambda node: 1 if node in c.soma[0] or node in c2.soma[0] 
                                  else 0, d=0.5, charge=2)

    h.finitialize(-65)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol

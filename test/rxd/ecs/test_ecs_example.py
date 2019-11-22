import pytest
from testutils import tol, compare_data

@pytest.fixture
def ecs_example(neuron_instance):
    from math import pi
    h, rxd, data = neuron_instance
    # create cell1 where `x` will be created and leak out
    cell1 = h.Section('cell1')
    cell1.pt3dclear()
    cell1.pt3dadd(-20,0,0,10)
    cell1.pt3dadd(-10,0,0,10)
    cell1.nseg = 11
    cell1.insert('pump')
    
    # create cell2 where `x` will be pumped in and accumulate
    cell2 = h.Section('cell2')
    cell2.pt3dclear()
    cell2.pt3dadd(10,0,0,10)
    cell2.pt3dadd(20,0,0,10)
    cell2.nseg = 11
    cell2.insert('pump')
    
    # Where?
    # the intracellular spaces
    cyt = rxd.Region(h.allsec(), name='cyt', nrn_region='i', geometry=rxd.FractionalVolume(0.9, surface_fraction=1.0))
    
    org = rxd.Region(h.allsec(), name='org', geometry=rxd.FractionalVolume(0.1))
    
    cyt_org_membrane = rxd.Region(h.allsec(), name='mem', geometry = rxd.ScalableBorder(pi/2.0, on_cell_surface=False))
    
    
    # the extracellular space
    ecs = rxd.Extracellular(-55, -55, -55, 55, 55, 55, dx=10, volume_fraction=0.2, tortuosity=1.6)
    
    # Who?
    x = rxd.Species([cyt, org, cyt_org_membrane, ecs], name='x', d=1.0, charge=1, initial=0)
    Xcyt = x[cyt]
    Xorg = x[org]
    
    # What? - produce X in cell 1
    # parameter to limit production to cell 1
    cell1_param = rxd.Parameter(org, initial=lambda node: 1.0 if node.segment.sec == cell1 else 0)
    
    # production with a rate following Michaels Menton kinetics
    createX = rxd.Rate(Xorg, cell1_param[org] * 1.0/(10.0 + Xorg))
    
    # leak between organelles and cytosol
    cyt_org_leak = rxd.MultiCompartmentReaction(Xcyt, Xorg, 1e4, 1e4, membrane=cyt_org_membrane)
    model = (cell1, cell2, cyt, org, cyt_org_membrane, ecs, x, Xcyt, Xorg, createX, cell1_param, createX, cyt_org_leak) 
    yield (neuron_instance, model)

def test_ecs_example(ecs_example):
    (h, rxd, data), model = ecs_example
    h.finitialize(-65)
    h.continuerun(1000)

    max_err = compare_data(data) 
    assert(max_err < tol)


def test_ecs_example_cvode(ecs_example):
    (h, rxd, data), model = ecs_example

    h.CVode().active(True)
    h.CVode().atol(1e-5)
    
    h.finitialize(-65)
    h.continuerun(1000)
    
    max_err = compare_data(data) 
    assert(max_err < tol)

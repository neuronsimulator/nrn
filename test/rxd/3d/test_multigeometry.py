import pytest
import numpy
from testutils import compare_data, tol


@pytest.mark.parametrize("hybrid", [True, False])
def test_multi_geometry3d(neuron_nosave_instance, hybrid):
    """A model using multiple different geometries in a 3D model"""

    h, rxd, save_path = neuron_nosave_instance
    dendA = h.Section(name="dendA")
    dendA.diam = 1
    dendA.nseg = 3
    dendA.L = 5
    dendB = h.Section(name="dendB")
    dendB.diam = 1
    dendB.nseg = 3
    dendB.L = 5

    if hybrid:
        rxd.set_solve_type([dendA], dimension=3)
    else:
        rxd.set_solve_type(dimension=3)

    geoA = rxd.inside
    geoB = rxd.FractionalVolume(volume_fraction=0.5, surface_fraction=1)
    geometry = rxd.MultipleGeometry(secs=[dendA, dendB], geos=[geoA, geoB])
    r = rxd.Region(h.allsec(), dx=0.75, geometry=geometry)
    ca = rxd.Species(r, name="ca", charge=2)

    if hybrid:
        assert len(ca.nodes(dendA)) == 28
        assert (len(ca.nodes(dendB))) == 3
        assert sum(ca.nodes(dendA).volume) == 2.671875
        assert sum(ca.nodes(dendB).volume) == 1.9634954084936205

    else:
        assert len(ca.nodes(dendA)) == 28
        assert len(ca.nodes(dendA)) == 28
        assert sum(ca.nodes(dendA).volume) == 2.671875
        assert sum(ca.nodes(dendB).volume) == 1.3359375

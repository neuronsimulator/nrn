from .rxdException import RxDException
from neuron import h
from . import geometry3d, dimension3
import copy
import itertools
import numpy
from . import geometry as geo

_region_count = 0

def _sort_secs(secs):
    # sort the sections
    root_secs = h.SectionList()
    root_secs.allroots()
    all_sorted = h.SectionList()
    for root in root_secs:
        all_sorted.wholetree(sec=root)
    secs_names = {sec.name():sec for sec in secs}
    for sec in secs:
        if h.section_orientation(sec=sec):
            raise RxDException('still need to deal with backwards sections')
    return [secs_names[sec.name()] for sec in all_sorted if sec.name() in secs_names]


# TODO: remove the need for this
_sim_dimension = None

class Region:
    def __repr__(self):
        return 'Region(..., nrn_region=%r, geometry=%r, dimension=%r, dx=%r)' % (self.nrn_region, self._geometry, _sim_dimension, self._dx)
    
    def __init__(self, sections, nrn_region=None, geometry=None, dimension=1, dx=None):
        global _region_count, _sim_dimension
        # TODO: validate sections (should be list of nrn.Section)
        if dimension == 3:
            if hasattr(sections, 'sections'):
                # TODO: Import3D. Won't work with currents, but no problem since need to remove anyways
                self._secs = sections #copy.copy(sections)
            else:
                self._secs = _sort_secs(sections)
            
        else:
            self._secs = _sort_secs(sections)
        if nrn_region not in (None, 'i', 'o'):
            raise RxDException('nrn_region must be one of: None, "i", "o"')
        self._nrn_region = nrn_region
        if dimension == 3 and geometry is not None:        
            raise RxDException('custom geometries not yet supported in 3d mode')
        if _sim_dimension is not None and dimension != _sim_dimension:
            raise RxDException('only one type of dimension per simulation supported for now (should change later)')
        _sim_dimension = dimension
        if geometry is None:
            geometry = geo.inside
        if dimension not in (1, 3):
            raise RxDException('only 1 and 3 dimensional simulations currently supported')
        if dimension != 3 and dx is not None:
            raise RxDException('dx option only accepted if dimension = 3')
        if dimension == 3 and dx is None:
            dx = 0.25
        self._geometry = geometry
        
        self._id = _region_count
        _region_count += 1
        self._dimension = dimension
        if dimension == 3:
            if nrn_region == 'o':
                raise RxDException('3d version does not support nrn_region="o" yet')

            self._mesh, sa, vol, self._tri = geometry3d.voxelize2(self._secs, dx=dx)
            sa_values = sa.values
            vol_values = vol.values
            self._objs = {}
            # TODO: remove this when can store soma outlines
            if not hasattr(sections, 'sections'):
                for sec in self._secs:
                    self._objs.update(dimension3.centroids_by_segment(sec))
            mesh_values = self._mesh.values
            xs, ys, zs = mesh_values.nonzero()
            mesh_xs, mesh_ys, mesh_zs = self._mesh._xs, self._mesh._ys, self._mesh._zs
            segs = []
            on_surface = []
            nodes_by_seg = {}
            surface_nodes_by_seg = {}

            # process surface area info
            self._sa = numpy.array([sa_values[x, y, z] for x, y, z in zip(xs, ys, zs)])
            on_surface = self._sa != 0
            
            # volumes
            self._vol = numpy.array([vol_values[x, y, z] for x, y, z in zip(xs, ys, zs)])
            
            # map each node to a segment
            for x, y, z, is_surf in zip(xs, ys, zs, on_surface):
                # compute distances to all objects to figure out which one
                # is closest
                # TODO: be smarter about this: use the chunkification code
                #       from geometry3d
                closest = None
                closest_dist = float('inf')
                myx, myy, myz = mesh_xs[x], mesh_ys[y], mesh_zs[z]
                for s, obs in zip(self._objs.keys(), self._objs.values()):
                    for o in obs:
                        dist = o.distance(myx, myy, myz)
                        if dist < closest_dist:
                            closest = s
                            closest_dist = dist
                seg = closest
                segs.append(seg)
                
                # TODO: predeclare these so don't have to check each time
                if seg not in nodes_by_seg:
                    nodes_by_seg[seg] = []
                    surface_nodes_by_seg[seg] = []
                nodes_by_seg[seg].append(len(segs) - 1)

                if is_surf:
                    surface_nodes_by_seg[seg].append(len(segs) - 1)
                    
            self._surface_nodes_by_seg = surface_nodes_by_seg
            self._nodes_by_seg = nodes_by_seg
            self._on_surface = on_surface
            self._xs = xs
            self._ys = ys
            self._zs = zs
            self._segs = segs            
        self._dx = dx

    @property
    def nrn_region(self):
        return self._nrn_region


    @property
    def dimension(self):
        return self._dimension
    
    @property
    def _semi_compile(self):
        return 'r%d' % self._id
    
    @property
    def secs(self):
        # return a copy of the section list
        return copy.copy(self._secs)
    

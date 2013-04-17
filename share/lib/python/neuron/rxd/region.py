from neuron import h
import geometry as geo
import copy
import geometry3d
import dimension3
import itertools

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
            raise Exception('still need to deal with backwards sections')
    return [secs_names[sec.name()] for sec in all_sorted if sec.name() in secs_names]


# TODO: remove the need for this
_sim_dimension = None

class Region:
    def __init__(self, sections, nrn_region=None, geometry=None, dimension=1, dx=None):
        global _region_count, _sim_dimension
        # TODO: validate sections (should be list of nrn.Section)
        if dimension == 3:
            self._secs = sections #copy.copy(sections)
        else:
            self._secs = _sort_secs(sections)
        if nrn_region not in (None, 'i', 'o'):
            raise Exception('nrn_region must be one of: None, "i", "o"')
        self._nrn_region = nrn_region
        if dimension == 3 and geometry is not None:        
            raise Exception('custom geometries not yet supported in 3d mode')
        if _sim_dimension is not None and dimension != _sim_dimension:
            raise Exception('only one type of dimension per simulation supported for now (should change later)')
        _sim_dimension = dimension
        if geometry is None:
            geometry = geo.inside
        if dimension not in (1, 3):
            raise Exception('only 1 and 3 dimensional simulations currently supported')
        if dimension != 3 and dx is not None:
            raise Exception('dx option only accepted if dimension = 3')
        if dimension == 3 and dx is None:
            dx = 0.25
        self._geometry = geometry
        
        self._id = _region_count
        _region_count += 1
        self._dimension = dimension
        if dimension == 3:
            if nrn_region == 'o':
                raise Exception('3d version does not support nrn_region="o" yet')

            self._mesh = geometry3d.voxelize(self._secs, dx=dx)
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
            for x, y, z in zip(xs, ys, zs):
                # check to see if the node is a surface node
                # (nodes are surface nodes if at least one of their neighbors is
                # not in the geometry)

                for a, b, c in itertools.product([x - 1, x, x + 1], [y - 1, y, y + 1], [z - 1, z, z + 1]):
                    try:
                        if not mesh_values[a, b, c]:
                            on_surface.append(False)
                            break
                    except IndexError:
                        pass
                else:
                    on_surface.append(True)


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

                if on_surface[-1]:
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
    

from .rxdException import RxDException
from neuron import h
try:
  from . import geometry3d, dimension3
except:
  pass
import copy
import itertools
import numpy
from . import geometry as geo
import weakref
from . import initializer
import warnings

_all_regions = []
_region_count = 0

def _sort_secs(secs):
    # sort the sections
    root_secs = h.SectionList()
    root_secs.allroots()
    all_sorted = h.SectionList()
    for root in root_secs:
        all_sorted.wholetree(sec=root)
    secs_names = dict([(sec.hoc_internal_name(),sec) for sec in secs])
    for sec in secs:
        if h.section_orientation(sec=sec):
            raise RxDException('still need to deal with backwards sections')
    return [secs_names[sec.hoc_internal_name()] for sec in all_sorted if sec.hoc_internal_name() in secs_names]

class Region(object):
    """Declare a conceptual region of the neuron.
    
    Examples: Cytosol, ER, extracellular space
    """
    def __repr__(self):
        # Note: this used to print out dimension, but that's now on a per-segment basis
        # TODO: remove the note when that is fully true
        self._dx = None
        return 'Region(..., nrn_region=%r, geometry=%r, dx=%r, name=%r)' % (self.nrn_region, self._geometry, self._dx, self._name)

    def _short_repr(self):
        if self._name is not None:
            return 'Region(<%r>)' % self._name
        else:
            return self.__repr__()

    
    def _do_init(self):
        global _region_count
        
        del self._allow_setting
        
        from . import rxd
        
        # parameters that were defined in old init
        # TODO: remove need for this bit
        nrn_region = self.nrn_region


        # TODO: self.dx needs to be removed... eventually that should be on a per-section basis
        #       right now, it has to be consistent but this is unenforced
        if self.dx is None:
            self.dx = 0.25
        dx = self.dx
        
        self._secs1d = []
        self._secs3d = []
        
        dims = rxd._dimensions
        for sec in self._secs:
            dim = dims[sec]
            if dim == 1:
                self._secs1d.append(sec)
            elif dim == 3:
                self._secs3d.append(sec)
            else:
                raise RxDException('unknown dimension: %r in section %r' % (dim, sec.name()))

        
        # TODO: I used to not sort secs in 3D if hasattr(self._secs, 'sections'); figure out why
        self._secs = _sort_secs(self._secs)
        self._secs1d = _sort_secs(self._secs1d)
        
        if self._secs3d and not(hasattr(self._geometry, 'volumes3d')):
            raise RxDException('selected geometry (%r) does not support 3d mode' % self._geometry)
        
        self._id = _region_count
        _region_count += 1
        if self._secs3d:
            if nrn_region == 'o':
                raise RxDException('3d simulations do not support nrn_region="o" yet')

            self._mesh, sa, vol, self._tri = self._geometry.volumes3d(self._secs3d, dx=dx)
            sa_values = sa.values
            vol_values = vol.values
            self._objs = {}
            # TODO: handle soma outlines correctly
            if not hasattr(self.secs, 'sections'):
                for sec in self._secs3d:
                    # NOTE: previously used centroids_by_segment instead
                    #       but that did bad things with spines
                    self._objs.update(dimension3.objects_by_segment(sec))
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
                for s, obs in zip(list(self._objs.keys()), list(self._objs.values())):
                    for o in obs:
                        # _distance is like distance except ignores end plates
                        # when inside
                        dist = o._distance(myx, myy, myz)
                        if dist < closest_dist:
                            closest = s
                            closest_dist = dist
                seg = closest
                segs.append(seg)
                
                # TODO: predeclare these so don't have to check each time
                # TODO: don't use segments; use internal names to avoid keeping sections alive
                if seg not in nodes_by_seg:
                    nodes_by_seg[seg] = []
                    surface_nodes_by_seg[seg] = []
                nodes_by_seg[seg].append(len(segs) - 1)

                if is_surf:
                    surface_nodes_by_seg[seg].append(len(segs) - 1)
            
            # NOTE: This stuff is for 3D part only
            self._surface_nodes_by_seg = surface_nodes_by_seg
            self._nodes_by_seg = nodes_by_seg
            self._on_surface = on_surface
            self._xs = xs
            self._ys = ys
            self._zs = zs
            # TODO: don't do this! This might keep the section alive!
            self._segs = segs
        self._dx = self.dx
    
    def _indices_from_sec_x(self, sec, position):
        # TODO: the assert is here because the diameter is not computed correctly
        #       unless it coincides with a 3d point, which we only know to exist at the
        #       endpoints and because the section does not proceed linearly between
        #       the endpoints (in general)... which affects the computation of the
        #       normal vector as well
        assert(position in (0, 1))
        # NOTE: some care is necessary in constructing normal vector... must be
        #       based on end frusta, not on vector between end points
        if position == 0:
            x = h.x3d(0, sec=sec)
            y = h.y3d(0, sec=sec)
            z = h.z3d(0, sec=sec)
            nx = h.x3d(1, sec=sec) - x
            ny = h.y3d(1, sec=sec) - y
            nz = h.z3d(1, sec=sec) - z
        elif position == 1:
            n = int(h.n3d(sec=sec))
            x = h.x3d(n - 1, sec=sec)
            y = h.y3d(n - 1, sec=sec)
            z = h.z3d(n - 1, sec=sec)
            # NOTE: sign of the normal is irrelevant
            nx = x - h.x3d(n - 2, sec=sec)
            ny = y - h.y3d(n - 2, sec=sec)
            nz = z - h.z3d(n - 2, sec=sec)
        else:
            raise RxDException('should never get here')
        # x, y, z = x * x1 + (1 - x) * x0, x * y1 + (1 - x) * y0, x * z1 + (1 - x) * z1
        r = sec(position).diam * 0.5
        plane_of_disc = geometry3d.graphicsPrimitives.Plane(x, y, z, nx, ny, nz)
        potential_coordinates = []
        mesh = self._mesh
        xs, ys, zs = mesh._xs, mesh._ys, mesh._zs
        xlo, ylo, zlo = xs[0], ys[0], zs[0]
        # locate the indices of the cube containing the sphere containing the disc
        # TODO: write this more efficiently
        i_indices = [i for i, a in enumerate(xs) if abs(a - x) < r]
        j_indices = [i for i, a in enumerate(ys) if abs(a - y) < r]
        k_indices = [i for i, a in enumerate(zs) if abs(a - z) < r]
        sphere_indices = [(i, j, k)
                          for i, j, k in itertools.product(i_indices, j_indices, k_indices)
                          if (xs[i] - x) ** 2 + (ys[j] - y) ** 2 + (zs[k] - z) ** 2 < r ** 2]
        dx2 = self.dx * 0.5
        dx = self.dx
        disc_indices = []
        for i, j, k in sphere_indices:
#            a, b, c = xs[i], ys[j], zs[k]
            # TODO: no need to compute all; can stop when some True and some False
#            on_side1 = [plane_of_disc.distance(x, y, z) >= 0 for x, y, z in itertools.product([a - dx2, a + dx2], [b - dx2, b + dx2], [c - dx2, c + dx2])]
            # NOTE: the expression is structured this way to make sure it tests the exact same corner coordinates for corners shared by multiple voxels and that there are no round-off issues (an earlier attempt had round-off issues that resulted in double-thick discs when the frustum ended exactly on a grid plane)
            on_side1 = [plane_of_disc.distance(x, y, z) >= 0 for x, y, z in itertools.product([(xlo + (i - 1) * dx) + dx2, (xlo + i * dx) + dx2], [(ylo + (j - 1) * dx) + dx2, (ylo + j * dx) + dx2], [(zlo + (k - 1) * dx) + dx2, (zlo + k * dx) + dx2])]
            # need both sides to have at least one corner
            if any(on_side1) and not all(on_side1):
                # if we're here, then we've found a point on the disc.
                disc_indices.append((i, j, k))
        return disc_indices
        
        
    
    def __init__(self, secs=None, nrn_region=None, geometry=None, dimension=None, dx=None, name=None):
        """
        In NEURON 7.4+, secs is optional at initial region declaration, but it
        must be specified before the reaction-diffusion model is instantiated.
        
        .. note:: dimension and dx will be deprecated in a future version
        """
        self._allow_setting = True
        self.secs = secs
        self.nrn_region = nrn_region
        self.geometry = geometry
        
        if dimension is not None:
            warnings.warn('dimension argument was a development feature only; use set_solve_type instead... the current version sets all the sections to your requested dimension, but this will override any previous settings')
            import neuron
            neuron.rxd.set_solve_type(secs, dimension=dimension)
        self._name = name
        self.dx = dx
        _all_regions.append(weakref.ref(self))

        # initialize self if the rest of rxd is already initialized
        if initializer.is_initialized():
            self._do_init()


    @property
    def nrn_region(self):
        """Get or set the classic NEURON region associated with this object.
        
        There are three possible values:
            * `'i'` -- just inside the plasma membrane
            * `'o'` -- just outside the plasma membrane
            * `None` -- none of the above
        
        .. note:: Setting only supported in NEURON 7.4+, and then only before the reaction-diffusion model is instantiated.
        """        
        return self._nrn_region
    
    @nrn_region.setter
    def nrn_region(self, value):
        if hasattr(self, '_allow_setting'):
            if value not in (None, 'i', 'o'):
                raise RxDException('nrn_region must be one of: None, "i", "o"')
            else:
                self._nrn_region = value
        else:
            raise RxDException('Cannot set nrn_region now; model already instantiated')

    @property
    def geometry(self):
        """Get or set the geometry associated with this region.
        
        Setting the geometry to `None` will cause it to default to `rxd.geometry.inside`.
        
        .. note:: New in NEURON 7.4+. Setting allowed only before the reaction-diffusion model is instantiated.
        """        
        return self._geometry

    @geometry.setter
    def geometry(self, value):
        if hasattr(self, '_allow_setting'):
            if value is None:
                value = geo.inside
            self._geometry = value
        else:
            raise RxDException('Cannot set geometry now; model already instantiated')

    @property
    def name(self):
        """Get or set the Region's name.

        .. note:: New in NEURON 7.4+. 
        """        
        return self._name

    @name.setter
    def name(self, value):
        self._name = value
    
    @property
    def _semi_compile(self):
        return 'r%d' % self._id
    
    @property
    def secs(self):
        """Get or set the sections associated with this region.
        
        The sections may be expressed as a NEURON SectionList or as any Python
        iterable of sections.
        
        Note: The return value is a copy of the internal section list; modifying
              it will not change the Region.
        
        .. note:: Setting is new in NEURON 7.4+ and allowed only before the reaction-diffusion model is instantiated.
        """
        if hasattr(self._secs, '__len__'):
            return list(self._secs)
        else:
            return [self._secs]

    @secs.setter
    def secs(self, value):
        if hasattr(self, '_allow_setting'):
            self._secs = value
        else:
            raise RxDException('Cannot set secs now; model already instantiated')    

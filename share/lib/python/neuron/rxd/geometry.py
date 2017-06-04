import warnings
import numpy
from neuron import h, nrn
from .rxdException import RxDException
try:
    from neuron.rxd import geometry3d
    has_geometry3d = True
except ImportError:
    has_geometry3d = False

class RxDGeometry:
    def volumes1d(self, sec):
        raise RxDException('volume1d unimplemented')
    def surface_areas1d(self, sec):
        raise RxDException('surface_areas1d unimplemented')
    def neighbor_areas1d(self, sec):
        raise RxDException('neighbor_areas1d unimplemented')
    def is_volume(self):
        raise RxDException('is_volume unimplemented')
    def is_area(self):
        raise RxDException('is_area unimplemented')
    def __call__(self):
        """calling returns self to allow for rxd.inside or rxd.inside()"""
        return self
        

def _volumes1d(sec):
    if not isinstance(sec, nrn.Section):
        sec = sec._sec
    arc3d = [h.arc3d(i, sec=sec)
             for i in range(int(h.n3d(sec=sec)))]
    diam3d = [h.diam3d(i, sec=sec)
              for i in range(int(h.n3d(sec=sec)))]
    vols = numpy.zeros(sec.nseg)
    dx = sec.L / sec.nseg
    for iseg in range(sec.nseg):
        # get a list of all pts in the segment, including end points
        lo = iseg * dx
        hi = (iseg + 1) * dx
        pts = [lo] + [x for x in arc3d if lo < x < hi] + [hi]
        
        diams = numpy.interp(pts, arc3d, diam3d)
        
        # sum the volume of the constituent frusta
        volume = 0
        for i in range(len(pts) - 1):
            diam0, diam1 = diams[i : i + 2]
            pt0, pt1 = pts[i : i + 2]
            volume += numpy.pi * (pt1 - pt0) / 12. * (diam0 ** 2 + diam0 * diam1 + diam1 ** 2)
        vols[iseg] = volume

    return vols

def _make_surfacearea1d_function(scale):
    def result(sec):
        if not isinstance(sec, nrn.Section):
            sec = sec._sec
        arc3d = [h.arc3d(i, sec=sec)
                for i in range(int(h.n3d(sec=sec)))]
        diam3d = [h.diam3d(i, sec=sec)
                for i in range(int(h.n3d(sec=sec)))]
        sas = numpy.zeros(sec.nseg)
        dx = sec.L / sec.nseg
        for iseg in range(sec.nseg):
            # get a list of all pts in the segment, including end points
            lo = iseg * dx
            hi = (iseg + 1) * dx
            pts = [lo] + [x for x in arc3d if lo < x < hi] + [hi]
            
            diams = numpy.interp(pts, arc3d, diam3d)
            
            # sum the surface areas of the constituent frusta
            sa = 0
            for i in range(len(pts) - 1):
                diam0, diam1 = diams[i : i + 2]
                pt0, pt1 = pts[i : i + 2]
                sa += scale * 0.5 * (diam0 + diam1) * numpy.sqrt(0.25 * (diam0 - diam1) ** 2 + (pt1 - pt0) ** 2)
            sas[iseg] = sa
        return sas
    return result

def _make_perimeter_function(scale):
    def result(sec):
        if not isinstance(sec, nrn.Section):
            sec = sec._sec
            arc3d = [h.arc3d(i, sec=sec)
                     for i in range(int(h.n3d(sec=sec)))]
            diam3d = [h.diam3d(i, sec=sec)
                      for i in range(int(h.n3d(sec=sec)))]
            area_pos = numpy.linspace(0, sec.L, sec.nseg + 1)
            diams = numpy.interp(area_pos, arc3d, diam3d)
            return scale * diams  
    return result
        
_surface_areas1d = _make_surfacearea1d_function(numpy.pi)
_perimeter1d = _make_perimeter_function(numpy.pi)
    
def _neighbor_areas1d(sec):
    if not isinstance(sec, nrn.Section):
        sec = sec._sec
    arc3d = [h.arc3d(i, sec=sec)
             for i in range(int(h.n3d(sec=sec)))]
    diam3d = [h.diam3d(i, sec=sec)
              for i in range(int(h.n3d(sec=sec)))]
    area_pos = numpy.linspace(0, sec.L, sec.nseg + 1)
    diams = numpy.interp(area_pos, arc3d, diam3d)
    return numpy.pi * 0.25 * diams ** 2

def constant_function_per_length(value):
    return lambda sec: [value * sec.L / sec.nseg for i in range(sec.nseg)]

def constant_everywhere_1d(value):
    return lambda sec: value * numpy.ones(sec.nseg)

def constant_everywhere_plus_one_1d(value):
    return lambda sec: value * numpy.ones(sec.nseg + 1)


def constant_function(value):
    return lambda *args, **kwargs: value

def scale_by_constant(scale, f):
    return lambda *args, **kwargs: scale * f(*args, **kwargs)

_always_true = constant_function(True)
_always_false = constant_function(False)
_always_0 = constant_function(0)

inside = RxDGeometry()
if has_geometry3d:
    inside.volumes3d = geometry3d.voxelize2
# neighbor_area_fraction can be a constant or a function
inside.neighbor_area_fraction = 1
inside.volumes1d = _volumes1d
inside.surface_areas1d = _surface_areas1d
inside.neighbor_areas1d = _neighbor_areas1d
inside.is_volume = _always_true
inside.is_area = _always_false
inside.__repr__ = constant_function('inside')

# TODO: make a version that allows arbitrary shells?
membrane = RxDGeometry()
membrane.volumes1d = _surface_areas1d
membrane.surface_areas1d = _always_0
membrane.neighbor_areas1d = _perimeter1d
membrane.is_volume = _always_false
membrane.is_area = _always_true
membrane.__repr__ = constant_function('membrane')

class Enum:
    """a silly way of creating unique identifiers without using/allowing/requiring magic constants"""
    pass

_lo_hi_shell = Enum()


class DistributedBoundary(RxDGeometry):
    """Boundary that scales with area.
    
    DistributedBoundary(area_per_vol, perim_per_area=0)
    
    area_per_vol is the area of the boundary as a function of the volume
    containing it. e.g.    
    
    g = DistributedBoundary(2) defines a geometry with 2 um^2 of area per
    every um^3 of volume.
    
    perim_per_area is the perimeter (in um) per 1 um^2 cross section of the
    volume. For use in reaction-diffusion problems, it may be safely omitted
    if and only if no species in the corresponding region diffuses.
    
    This is often useful for separating FractionalVolume objects.
    
    It is assumed that the area is always strictly on the interior.
    """
    def __init__(self, area_per_vol, perim_per_area=0):
        self._area_per_vol = area_per_vol
        self._perim_per_area = 0

        self.surface_areas1d = _always_0
        self.neighbor_areas1d = scale_by_constant(perim_per_area, _neighbor_areas1d)
        self.volumes1d = scale_by_constant(area_per_vol, _volumes1d)
        self.is_volume = _always_false
        self.is_area = _always_true
    
    @property
    def neighbor_area_fraction(self):
        # TODO: validate that this gives consistent results between 1D and 3D
        return self._perim_per_area
        
    def volumes3d(self, source, dx=0.25, xlo=None, xhi=None, ylo=None, yhi=None, zlo=None, zhi=None, n_soma_step=100):
        mesh, surface_areas, volumes, triangles = geometry3d.voxelize2(source, dx=dx, xlo=xlo, xhi=xhi, ylo=ylo, yhi=yhi, zlo=zlo, zhi=zhi, n_soma_step=n_soma_step)
        volumes._values *= self._area_per_vol # volume on 2D boundaries is actually the area; the amount of space for holding things
        surface_areas._values *= 0 
        return mesh, surface_areas, volumes, triangles
    
    def __repr__(self):
        if self._perim_per_area == 0:
            return 'DistributedBoundary(%g)' % (self._area_per_vol)
        else:
            return 'DistributedBoundary(%g, perim_per_area=%g)' % (self._area_per_vol, self._perim_per_area)




class FractionalVolume(RxDGeometry):
    def __init__(self, volume_fraction=1, surface_fraction=0, neighbor_areas_fraction=None):
        if neighbor_areas_fraction is None:
            neighbor_areas_fraction = volume_fraction
        if surface_fraction == 0:
            self.surface_areas1d = _always_0
        elif surface_fraction == 1:
            self.surface_areas1d = _surface_areas1d
        else:
            self.surface_areas1d = scale_by_constant(surface_fraction, _surface_areas1d)
        # TODO: add the if statement so not scaling if 0 or 1
        self.neighbor_areas1d = scale_by_constant(neighbor_areas_fraction, _neighbor_areas1d)
        self.volumes1d = scale_by_constant(volume_fraction, _volumes1d)
        self.is_volume = _always_true
        self.is_area = _always_false
        self._volume_fraction = volume_fraction
        self._surface_fraction = surface_fraction
        self._neighbor_areas_fraction = neighbor_areas_fraction
        # TODO: does the else case ever make sense?
        self.neighbor_area_fraction = volume_fraction if neighbor_areas_fraction is None else neighbor_areas_fraction
        
    def volumes3d(self, source, dx=0.25, xlo=None, xhi=None, ylo=None, yhi=None, zlo=None, zhi=None, n_soma_step=100):
        mesh, surface_areas, volumes, triangles = geometry3d.voxelize2(source, dx=dx, xlo=xlo, xhi=xhi, ylo=ylo, yhi=yhi, zlo=zlo, zhi=zhi, n_soma_step=n_soma_step)
        surface_areas._values *= self._surface_fraction
        volumes._values *= self._volume_fraction
        return mesh, surface_areas, volumes, triangles
    
    def __repr__(self):
        return 'FractionalVolume(volume_fraction=%r, surface_fraction=%r, neighbor_areas_fraction=%r)' % (self._volume_fraction, self._surface_fraction, self._neighbor_areas_fraction)

# TODO: eliminate this class and replace with FixedCrossSection?
class ConstantVolume(RxDGeometry):
    # TODO: do we want different default neighbor_area?
    def __init__(self, volume=1, surface_area=0, neighbor_area=1):
        """volume, surface_area per unit length"""
        self.volumes1d = constant_function_per_length(volume)
        self.surface_areas1d = constant_function_per_length(surface_area)
        self.is_volume = _always_true
        self.is_area = _always_false
        self.neighbor_areas1d = constant_everywhere_plus_one_1d(neighbor_area)
    



class FixedCrossSection(RxDGeometry):
    def __init__(self, cross_area, surface_area=0):
        self.volumes1d = constant_function_per_length(cross_area)
        self.surface_areas1d = constant_function_per_length(surface_area)
        self.is_volume = _always_true
        self.is_area = _always_false
        self.neighbor_areas1d = constant_everywhere_plus_one_1d(cross_area)
        self._cross_area = cross_area
        self._surface_area = surface_area
        
    def __repr__(self):
        return 'FixedCrossSection(%r, surface_area=%r)' % (self._cross_area, self._surface_area)
    
class FixedPerimeter(RxDGeometry):
    def __init__(self, perimeter, on_cell_surface=False):
        self.volumes1d = constant_function_per_length(perimeter)
        self.surface_areas1d = _always_0 if not on_cell_surface else self.volumes1d
        self._perim = perimeter
        self.is_volume = _always_false
        self.is_area = _always_true
        self._on_surface = on_cell_surface
    def neighbor_areas1d(self, sec):
        return [self._perim] * (sec.nseg + 1)

    def __repr__(self):
        return 'FixedPerimeter(%r, on_cell_surface=%r)' % (self._perim, self._on_surface)

class ScalableBorder(RxDGeometry):
    """a membrane that scales proportionally with the diameter
    
    Example use:
    
    - the boundary between radial shells
    
    Sometimes useful for the boundary between FractionalVolume objects, but
    see also DistributedBoundary which scales with area.
    """
    def __init__(self, scale, on_cell_surface=False):
        self.volumes1d = _make_surfacearea1d_function(scale)
        self.surface_areas1d = _always_0 if not on_cell_surface else self.volumes1d
        self._scale = scale
        self.is_volume = _always_false
        self.is_area = _always_true
        self.neighbor_areas1d = _make_perimeter_function(scale)
        self._on_surface = on_cell_surface
    def __repr__(self):
        return 'ScalableBorder(%r, on_cell_surface=%r)' % (self._scale, self._on_surface)

# TODO: remove this, use FixedPerimeter instead?        
class ConstantArea(RxDGeometry):
    def __init__(self, area=1, perim=1, on_cell_surface=False):
        # TODO: fix this
        warnings.warn('ConstantArea probably not right')
        self.volumes1d = constant_function(area)
        self.surface_areas1d = _always_0 if not on_cell_surface else constant_function(area)
        self._perim = perim
        self.is_volume = _always_false
        self.is_area = _always_true
    def neighbor_areas1d(self, sec):
        return [self._perim] * (sec.nseg + 1)


# TODO: is there a better name than Shell?
class Shell(RxDGeometry):
    def __init__(self, lo=None, hi=None):
        if lo is None or hi is None:
            raise RxDException('only Shells with a lo and hi are supported for now')
        
        if lo > hi: lo, hi = hi, lo
        if lo == hi:
            raise RxDException('Shell objects must have thickness')
        self._type = _lo_hi_shell
        self._lo = lo
        self._hi = hi
        
        if lo == 1 or hi == 1:
            self.surface_areas1d = _surface_areas1d
        elif lo < 1 < hi:
            raise RxDException('shells may not cross the membrane; i.e. 1 cannot lie strictly between lo and hi')
        else:
            # TODO: is this what we want; e.g. what if lo < 1 < hi?
            self.surface_areas1d = _always_0
    
    def __repr__(self):
        return 'Shell(lo=%r, hi=%r)' % (self._lo, self._hi)
    
    def neighbor_areas1d(self, sec):
        if not isinstance(sec, nrn.Section):
            sec = sec._sec
        arc3d = [h.arc3d(i, sec=sec)
                 for i in range(int(h.n3d(sec=sec)))]
        diam3d = [h.diam3d(i, sec=sec)
                  for i in range(int(h.n3d(sec=sec)))]
        area_pos = numpy.linspace(0, sec.L, sec.nseg + 1)
        diams = numpy.interp(area_pos, arc3d, diam3d)
        if self._type == _lo_hi_shell:
            return numpy.pi * .25 * ((diams * self._hi) ** 2 - (diams * self._lo) ** 2)
    
    def is_volume(self): return True
    def is_area(self): return False
    
    def volumes1d(self, sec):
        if not isinstance(sec, nrn.Section):
            sec = sec._sec
        arc3d = [h.arc3d(i, sec=sec)
                 for i in range(int(h.n3d(sec=sec)))]
        diam3d = [h.diam3d(i, sec=sec)
                  for i in range(int(h.n3d(sec=sec)))]
        vols = numpy.zeros(sec.nseg)
        dx = sec.L / sec.nseg
        for iseg in range(sec.nseg):
            # get a list of all pts in the segment, including end points
            lo = iseg * dx
            hi = (iseg + 1) * dx
            pts = [lo] + [x for x in arc3d if lo < x < hi] + [hi]
            
            diams = numpy.interp(pts, arc3d, diam3d)
            
            # sum the volume of the constituent frusta, hollowing out by the inside
            volume = 0
            for i in range(len(pts) - 1):
                diam0h, diam1h = self._hi * diams[i : i + 2]
                diam0l, diam1l = self._lo * diams[i : i + 2]
                pt0, pt1 = pts[i : i + 2]
                volume += numpy.pi * (pt1 - pt0) / 12. * ((diam0h ** 2 + diam0h * diam1h + diam1h ** 2) - (diam0l ** 2 + diam0l * diam1l + diam1l ** 2))
            vols[iseg] = volume

        return vols

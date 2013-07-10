import warnings
import numpy
from neuron import h, nrn

class RxDGeometry:
    def volumes1d(self, sec):
        raise Exception('volume1d unimplemented')
    def surface_areas1d(self, sec):
        raise Exception('surface_areas1d unimplemented')
    def neighbor_areas1d(self, sec):
        raise Exception('neighbor_areas1d unimplemented')
    def is_volume(self):
        raise Exception('is_volume unimplemented')
    def is_area(self):
        raise Exception('is_area unimplemented')
    def __call__(self):
        """calling returns self to allow for rxd.inside or rxd.inside()"""
        return self
        

def _volumes1d(sec):
    arc3d = [h.arc3d(i, sec=sec._sec)
             for i in xrange(int(h.n3d(sec=sec._sec)))]
    diam3d = [h.diam3d(i, sec=sec._sec)
              for i in xrange(int(h.n3d(sec=sec._sec)))]
    vols = numpy.zeros(sec.nseg)
    dx = sec.L / sec.nseg
    for iseg in xrange(sec.nseg):
        # get a list of all pts in the segment, including end points
        lo = iseg * dx
        hi = (iseg + 1) * dx
        pts = [lo] + [x for x in arc3d if lo < x < hi] + [hi]
        
        diams = numpy.interp(pts, arc3d, diam3d)
        
        # sum the volume of the constituent frusta
        volume = 0
        for i in xrange(len(pts) - 1):
            diam0, diam1 = diams[i : i + 2]
            pt0, pt1 = pts[i : i + 2]
            volume += numpy.pi * (pt1 - pt0) / 12. * (diam0 ** 2 + diam0 * diam1 + diam1 ** 2)
        vols[iseg] = volume

    return vols


def _surface_areas1d(sec):
    if not isinstance(sec, nrn.Section):
        sec = sec._sec
    arc3d = [h.arc3d(i, sec=sec)
             for i in xrange(int(h.n3d(sec=sec)))]
    diam3d = [h.diam3d(i, sec=sec)
              for i in xrange(int(h.n3d(sec=sec)))]
    sas = numpy.zeros(sec.nseg)
    dx = sec.L / sec.nseg
    for iseg in xrange(sec.nseg):
        # get a list of all pts in the segment, including end points
        lo = iseg * dx
        hi = (iseg + 1) * dx
        pts = [lo] + [x for x in arc3d if lo < x < hi] + [hi]
        
        diams = numpy.interp(pts, arc3d, diam3d)
        
        # sum the surface areas of the constituent frusta
        sa = 0
        for i in xrange(len(pts) - 1):
            diam0, diam1 = diams[i : i + 2]
            pt0, pt1 = pts[i : i + 2]
            sa += numpy.pi * 0.5 * (diam0 + diam1) * numpy.sqrt(0.25 * (diam0 - diam1) ** 2 + (pt1 - pt0) ** 2)
        sas[iseg] = sa
    return sas
    
def _perimeter1d(sec):
    if not isinstance(sec, nrn.Section):
        sec = sec._sec
    arc3d = [h.arc3d(i, sec=sec)
             for i in xrange(int(h.n3d(sec=sec)))]
    diam3d = [h.diam3d(i, sec=sec)
              for i in xrange(int(h.n3d(sec=sec)))]
    area_pos = numpy.linspace(0, sec.L, sec.nseg + 1)
    diams = numpy.interp(area_pos, arc3d, diam3d)
    return numpy.pi * diams    
    
def _neighbor_areas1d(sec):
    arc3d = [h.arc3d(i, sec=sec._sec)
             for i in xrange(int(h.n3d(sec=sec._sec)))]
    diam3d = [h.diam3d(i, sec=sec._sec)
              for i in xrange(int(h.n3d(sec=sec._sec)))]
    area_pos = numpy.linspace(0, sec.L, sec.nseg + 1)
    diams = numpy.interp(area_pos, arc3d, diam3d)
    return numpy.pi * 0.25 * diams ** 2

def constant_function_per_length(value):
    return lambda sec: [value * sec.L / sec.nseg for i in xrange(sec.nseg)]

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
inside.volumes1d = _volumes1d
inside.surface_areas1d = _surface_areas1d
inside.neighbor_areas1d = _neighbor_areas1d
inside.is_volume = _always_true
inside.is_area = _always_false

# TODO: make a version that allows arbitrary shells?
membrane = RxDGeometry()
membrane.volumes1d = _surface_areas1d
membrane.surface_areas1d = _always_0
membrane.neighbor_areas1d = _perimeter1d
membrane.is_volume = _always_false
membrane.is_area = _always_true

class Enum:
    """a silly way of creating unique identifiers without using/allowing/requiring magic constants"""
    pass

_lo_hi_shell = Enum()


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
    
class FixedPerimeter(RxDGeometry):
    def __init__(self, perimeter, on_cell_surface=False):
        self.volumes1d = constant_function_per_length(perimeter)
        self.surface_areas1d = _always_0 if not on_cell_surface else self.volumes1d
        self._perim = perimeter
        self.is_volume = _always_false
        self.is_area = _always_true
    def neighbor_areas1d(self, sec):
        return [self._perim] * (sec.nseg + 1)

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
            raise Exception('only Shells with a lo and hi are supported for now')
        
        if lo > hi: lo, hi = hi, lo
        if lo == hi:
            raise Exception('Shell objects must have thickness')
        self._type = _lo_hi_shell
        self._lo = lo
        self._hi = hi
        
        if hi == 1:
            self.surface_areas1d = _surface_areas1d
        elif lo <= 1 < hi:
            raise Exception('have not yet implemented support for lo <= 1 < hi')
        else:
            # TODO: is this what we want; e.g. what if lo < 1 < hi?
            self.surface_areas1d = _always_0
    
    def neighbor_areas1d(self, sec):
        arc3d = [h.arc3d(i, sec=sec._sec)
                 for i in xrange(int(h.n3d(sec=sec._sec)))]
        diam3d = [h.diam3d(i, sec=sec._sec)
                  for i in xrange(int(h.n3d(sec=sec._sec)))]
        area_pos = numpy.linspace(0, sec.L, sec.nseg + 1)
        diams = numpy.interp(area_pos, arc3d, diam3d)
        if self._type == _lo_hi_shell:
            return numpy.pi * .25 * ((diams * self._hi) ** 2 - (diams * self._lo) ** 2)
    
    def is_volume(self): return True
    def is_area(self): return False
    
    def volumes1d(self, sec):
        arc3d = [h.arc3d(i, sec=sec._sec)
                 for i in xrange(int(h.n3d(sec=sec._sec)))]
        diam3d = [h.diam3d(i, sec=sec._sec)
                  for i in xrange(int(h.n3d(sec=sec._sec)))]
        vols = numpy.zeros(sec.nseg)
        dx = sec.L / sec.nseg
        for iseg in xrange(sec.nseg):
            # get a list of all pts in the segment, including end points
            lo = iseg * dx
            hi = (iseg + 1) * dx
            pts = [lo] + [x for x in arc3d if lo < x < hi] + [hi]
            
            diams = numpy.interp(pts, arc3d, diam3d)
            
            # sum the volume of the constituent frusta, hollowing out by the inside
            volume = 0
            for i in xrange(len(pts) - 1):
                diam0h, diam1h = self._hi * diams[i : i + 2]
                diam0l, diam1l = self._lo * diams[i : i + 2]
                pt0, pt1 = pts[i : i + 2]
                volume += numpy.pi * (pt1 - pt0) / 12. * ((diam0h ** 2 + diam0h * diam1h + diam1h ** 2) - (diam0l ** 2 + diam0l * diam1l + diam1l ** 2))
            vols[iseg] = volume

        return vols

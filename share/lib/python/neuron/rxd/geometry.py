# -*- coding: utf-8 -*-
import warnings
import numpy
from neuron import h, nrn
from .rxdException import RxDException
from itertools import groupby

try:
    from neuron.rxd import geometry3d

    has_geometry3d = True
except ImportError:
    has_geometry3d = False


class RxDGeometry:
    def volumes1d(self, sec):
        raise RxDException("volume1d unimplemented")

    def surface_areas1d(self, sec):
        raise RxDException("surface_areas1d unimplemented")

    def neighbor_areas1d(self, sec):
        raise RxDException("neighbor_areas1d unimplemented")

    def is_volume(self):
        raise RxDException("is_volume unimplemented")

    def is_area(self):
        raise RxDException("is_area unimplemented")

    def __call__(self):
        """calling returns self to allow for rxd.inside or rxd.inside()"""
        return self


def _volumes1d(sec):
    if not isinstance(sec, nrn.Section):
        sec = sec._sec
    arc3d = [sec.arc3d(i) for i in range(sec.n3d())]
    diam3d = [sec.diam3d(i) for i in range(sec.n3d())]
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
            volume += (
                numpy.pi
                * (pt1 - pt0)
                / 12.0
                * (diam0**2 + diam0 * diam1 + diam1**2)
            )
        vols[iseg] = volume

    return vols


def _make_surfacearea1d_function(scale, diam_scale=1.0):
    def result(sec):
        if not isinstance(sec, nrn.Section):
            sec = sec._sec
        arc3d = [sec.arc3d(i) for i in range(sec.n3d())]
        diam3d = [sec.diam3d(i) * diam_scale for i in range(sec.n3d())]
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
                sa += (
                    scale
                    * 0.5
                    * (diam0 + diam1)
                    * numpy.sqrt(0.25 * (diam0 - diam1) ** 2 + (pt1 - pt0) ** 2)
                )
            sas[iseg] = sa
        return sas

    return result


def _make_perimeter_function(scale, diam_scale=1.0):
    def result(sec):
        if not isinstance(sec, nrn.Section):
            sec = sec._sec
        arc3d = [sec.arc3d(i) for i in range(sec.n3d())]
        diam3d = [sec.diam3d(i) * diam_scale for i in range(sec.n3d())]
        area_pos = numpy.linspace(0, sec.L, sec.nseg + 1)
        diams = numpy.interp(area_pos, arc3d, diam3d)
        return scale * diams

    return result


_surface_areas1d = _make_surfacearea1d_function(numpy.pi)
_perimeter1d = _make_perimeter_function(numpy.pi)


def _neighbor_areas1d(sec):
    if not isinstance(sec, nrn.Section):
        sec = sec._sec
    arc3d = [sec.arc3d(i) for i in range(sec.n3d())]
    diam3d = [sec.diam3d(i) for i in range(sec.n3d())]
    area_pos = numpy.linspace(0, sec.L, sec.nseg + 1)
    diams = numpy.interp(area_pos, arc3d, diam3d)
    return numpy.pi * 0.25 * diams**2


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
    inside._scale_internal_volumes3d = 1
    inside._scale_surface_volumes3d = 1
# neighbor_area_fraction can be a constant or a function
inside.neighbor_area_fraction = 1
inside.volumes1d = _volumes1d
inside.surface_areas1d = _surface_areas1d
inside.neighbor_areas1d = _neighbor_areas1d
inside.is_volume = _always_true
inside.is_area = _always_false
inside.__repr__ = constant_function("inside")

# TODO: make a version that allows arbitrary shells?
membrane = RxDGeometry()
membrane.volumes1d = _surface_areas1d
membrane.surface_areas1d = _always_0
membrane.neighbor_areas1d = _perimeter1d
membrane.is_volume = _always_false
membrane.is_area = _always_true
membrane.__repr__ = constant_function("membrane")


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
        self._scale_internal_volumes3d = area_per_vol
        self._scale_surface_volumes3d = area_per_vol
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

    def volumes3d(
        self,
        source,
        dx=0.25,
        xlo=None,
        xhi=None,
        ylo=None,
        yhi=None,
        zlo=None,
        zhi=None,
        n_soma_step=100,
    ):
        # mesh, surface_areas, volumes, triangles = geometry3d.voxelize2(source, dx=dx)
        # volumes._values *= self._area_per_vol # volume on 2D boundaries is actually the area; the amount of space for holding things
        # surface_areas._values *= 0
        # return mesh, surface_areas, volumes, triangles
        internal_voxels, surface_voxels, mesh_grid = geometry3d.voxelize2(source, dx=dx)
        area_per_vol = self._area_per_vol
        for key in internal_voxels:
            internal_voxels[key][0] *= area_per_vol
        for key in surface_voxels:
            surface_voxels[key][0] *= area_per_vol
        return internal_voxels, surface_voxels, mesh_grid

    def __repr__(self):
        if self._perim_per_area == 0:
            return f"DistributedBoundary({self._area_per_vol:g})"
        else:
            return f"DistributedBoundary({self._area_per_vol:g}, perim_per_area={self._perim_per_area:g})"


class FractionalVolume(RxDGeometry):
    def __init__(
        self, volume_fraction=1, surface_fraction=0, neighbor_areas_fraction=None
    ):
        if neighbor_areas_fraction is None:
            neighbor_areas_fraction = volume_fraction
        if surface_fraction == 0:
            self.surface_areas1d = _always_0
        elif surface_fraction == 1:
            self.surface_areas1d = _surface_areas1d
        else:
            self.surface_areas1d = scale_by_constant(surface_fraction, _surface_areas1d)
        # TODO: add the if statement so not scaling if 0 or 1
        self.neighbor_areas1d = scale_by_constant(
            neighbor_areas_fraction, _neighbor_areas1d
        )
        self.volumes1d = scale_by_constant(volume_fraction, _volumes1d)
        self.is_volume = _always_true
        self.is_area = _always_false
        self._volume_fraction = volume_fraction
        self._scale_internal_volumes3d = volume_fraction
        self._scale_surface_volumes3d = volume_fraction

        self._surface_fraction = surface_fraction
        self._neighbor_areas_fraction = neighbor_areas_fraction
        # TODO: does the else case ever make sense?
        self.neighbor_area_fraction = (
            volume_fraction
            if neighbor_areas_fraction is None
            else neighbor_areas_fraction
        )

    def volumes3d(
        self,
        source,
        dx=0.25,
        xlo=None,
        xhi=None,
        ylo=None,
        yhi=None,
        zlo=None,
        zhi=None,
        n_soma_step=100,
    ):
        # mesh, surface_areas, volumes, triangles = geometry3d.voxelize2(source, dx=dx)
        # surface_areas._values *= self._surface_fraction
        # volumes._values *= self._volume_fraction
        # return mesh, surface_areas, volumes, triangles
        internal_voxels, surface_voxels, mesh_grid = geometry3d.voxelize2(source, dx=dx)
        volume_fraction = self._volume_fraction
        for key in internal_voxels:
            internal_voxels[key][0] *= volume_fraction
        for key in surface_voxels:
            surface_voxels[key][0] *= volume_fraction
        return internal_voxels, surface_voxels, mesh_grid

    def __repr__(self):
        return f"FractionalVolume(volume_fraction={self._volume_fraction!r}, surface_fraction={self._surface_fraction!r}, neighbor_areas_fraction={self._neighbor_areas_fraction!r})"


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
        return f"FixedCrossSection({self._cross_area!r}, surface_area={self._surface_area!r})"


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
        return f"FixedPerimeter({self._perim!r}, on_cell_surface={self._on_surface!r})"


class ScalableBorder(RxDGeometry):
    """a membrane that scales proportionally with the diameter.

    Example use:

    - the boundary between radial shells e.g.
      ScalableBorder(diam_scale=0.5) could represent the border of
      Shell(lo=0, hi=0.5)


    Args:
        scale (float, optional) scale the area, default value is π.
            e.g. for a cylinder of length L and diameter d, ScalableBorder will
            give an area scale*d*L, by default the surface area.
            For cylindrical sections only. Use "diam_scale" instead to correctly
            handle cylindrical and non-cylindrical sections.
        diam_scale (float, optional), scale the diameter, default value is 1.
            e.g. for a cylinder of length L and diameter d, ScalableBorder will
            give an area diam_scale*π*d*L, by default the surface area.

    Note: Provide either a scale or diam_scale, not both.

    Sometimes useful for the boundary between FractionalVolume objects, but
    see also DistributedBoundary which scales with area.
    """

    def __init__(self, scale=None, diam_scale=None, on_cell_surface=False):
        if scale is not None and diam_scale is not None:
            raise RxDException(
                "ScalableBorder either provide scale or diam_scale, not both"
            )
        elif diam_scale is not None:
            self._scale = numpy.pi
            self._diam_scale = diam_scale
        elif scale is not None:
            self._scale = scale
            self._diam_scale = 1.0
        else:
            self._scale = numpy.pi
            self._diam_scale = 1.0

        self.volumes1d = _make_surfacearea1d_function(self._scale, self._diam_scale)
        self.surface_areas1d = _always_0 if not on_cell_surface else self.volumes1d
        self.is_volume = _always_false
        self.is_area = _always_true
        self.neighbor_areas1d = _make_perimeter_function(self._scale, self._diam_scale)
        self._on_surface = on_cell_surface

    def __repr__(self):
        return f"ScalableBorder({self._scale!r}, on_cell_surface={self._on_surface!r})"


# TODO: remove this, use FixedPerimeter instead?
class ConstantArea(RxDGeometry):
    def __init__(self, area=1, perim=1, on_cell_surface=False):
        # TODO: fix this
        warnings.warn("ConstantArea probably not right")
        self.volumes1d = constant_function(area)
        self.surface_areas1d = (
            _always_0 if not on_cell_surface else constant_function(area)
        )
        self._perim = perim
        self.is_volume = _always_false
        self.is_area = _always_true

    def neighbor_areas1d(self, sec):
        return [self._perim] * (sec.nseg + 1)


# TODO: is there a better name than Shell?
class Shell(RxDGeometry):
    def __init__(self, lo=None, hi=None):
        if lo is None or hi is None:
            raise RxDException("only Shells with a lo and hi are supported for now")

        if lo > hi:
            lo, hi = hi, lo
        if lo == hi:
            raise RxDException("Shell objects must have thickness")
        self._type = _lo_hi_shell
        self._lo = lo
        self._hi = hi

        if lo == 1 or hi == 1:
            self.surface_areas1d = _surface_areas1d
        elif lo < 1 < hi:
            raise RxDException(
                "shells may not cross the membrane; i.e. 1 cannot lie strictly between lo and hi"
            )
        else:
            # TODO: is this what we want; e.g. what if lo < 1 < hi?
            self.surface_areas1d = _always_0

    def __repr__(self):
        return f"Shell(lo={self._lo!r}, hi={self._hi!r})"

    def neighbor_areas1d(self, sec):
        if not isinstance(sec, nrn.Section):
            sec = sec._sec
        arc3d = [sec.arc3d(i) for i in range(sec.n3d())]
        diam3d = [sec.diam3d(i) for i in range(sec.n3d())]
        area_pos = numpy.linspace(0, sec.L, sec.nseg + 1)
        diams = numpy.interp(area_pos, arc3d, diam3d)
        if self._type == _lo_hi_shell:
            return numpy.pi * 0.25 * ((diams * self._hi) ** 2 - (diams * self._lo) ** 2)

    def is_volume(self):
        return True

    def is_area(self):
        return False

    def volumes1d(self, sec):
        if not isinstance(sec, nrn.Section):
            sec = sec._sec
        arc3d = [sec.arc3d(i) for i in range(sec.n3d())]
        diam3d = [sec.diam3d(i) for i in range(sec.n3d())]
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
                volume += (
                    numpy.pi
                    * (pt1 - pt0)
                    / 12.0
                    * (
                        (diam0h**2 + diam0h * diam1h + diam1h**2)
                        - (diam0l**2 + diam0l * diam1l + diam1l**2)
                    )
                )
            vols[iseg] = volume

        return vols


class MultipleGeometry(RxDGeometry):
    """support for different geometries on different sections of a region.

    Example use:
    - for radial diffusion in a dendrite (dend) with longitudinal diffusion
      from a spine (spine). The region for the outer shell of the dendrite
      (0.8,1] should include the while spine [0,1];
      MultipleGeometry(secs=[dend,spine], geos=[Shell(0.8,1), rxd.inside])

    Args:
    sections (list, optional) a list or list-of-lists of sections where the
        corresponding geometry should be used. If None the same geometry used
        for all sections, otherwise the list must be the same length as the
        geos list.
        If None is in the list, then the corresponding geometry in geos is used
        as a default value for any section not included in the lists.
    geometries (list) a list of geometries that are used for the corresponding
        list of sections in secs. All geometries must be volumes or all
        all geometries must be areas.

    """

    def __init__(self, secs=None, geos=None):
        self._secs = {}
        self._default = None
        if not secs:
            if isinstance(geos, list):
                self._default = geos[0]
            elif isinstance(geos, RxDGeometry):
                self._default = geos
            else:
                raise RxDException(
                    "MultipleGeometry requires a list-of-lists of sections and their corresponding geometry"
                )
        else:
            assert len(secs) == len(geos)
            if all([g.is_area() for g in geos]):
                self.is_area = _always_true
                self.is_volume = _always_false
            elif all([g.is_volume() for g in geos]):
                self.is_area = _always_false
                self.is_volume = _always_true
            else:
                raise RxDException(
                    "MultipleGeometry requires all geometries are areas or all geometries are volumes"
                )
            for s, g in zip(secs, geos):
                if not s:
                    self._default = g
                elif isinstance(s, list):
                    self._secs[h.SectionList(s)] = g
                else:
                    self._secs[h.SectionList([s])] = g

    def __repr__(self):
        secs = [[s for s in sl] for sl in self._secs]
        geos = [self._secs[sl] for sl in self._secs]
        return f"MultipleGeometry(secs={secs!r}, geos={geos!r})"

    def _get_geo(self, sec):
        if not isinstance(sec, nrn.Section):
            sec = sec._sec
        for sl in self._secs:
            if sec in sl:
                geo = self._secs[sl]
                break
        else:
            if self._default:
                geo = self._default
            else:
                raise RxDException(
                    f"MultipleGeometry is not defined on section {sec!r}"
                )
        return geo

    def volumes3d(self, source, dx=0.25):
        geos = {}
        for sec in source:
            geo = self._get_geo(sec)
            geos.setdefault(geo, []).append(sec)
        internal_voxels, surface_voxels, mesh_grid = geometry3d.voxelize2(source, dx)
        internal_key = lambda itm: itm[1][1].sec
        surface_key = lambda itm: itm[1][2].sec
        internal_by_sec = groupby(internal_voxels.items(), key=internal_key)
        surface_by_sec = groupby(surface_voxels.items(), key=surface_key)
        for geo, secs in geos.items():
            # only scale if not multiplying by 1
            if geo._scale_internal_volumes3d != 1:
                for sec, voxels in internal_by_sec:
                    # only scale if sec is in this geometry
                    if sec in secs:
                        # scale the internal volume
                        for key, _ in voxels:
                            internal_voxels[key][0] *= geo._scale_internal_volumes3d
            # only scale if not multiplying by 1
            if geo._scale_surface_volumes3d != 1:
                for sec, voxels in surface_by_sec:
                    # only scale if sec is in this geometry
                    if sec in secs:
                        # scale the surface volume
                        for key, _ in voxels:
                            surface_voxels[key][0] *= geo._scale_surface_volumes3d

        return internal_voxels, surface_voxels, mesh_grid

    def volumes1d(self, sec):
        return self._get_geo(sec).volumes1d(sec)

    def surface_areas1d(self, sec):
        return self._get_geo(sec).surface_areas1d(sec)

    def neighbor_areas1d(self, sec):
        return self._get_geo(sec).neighbor_areas1d(sec)

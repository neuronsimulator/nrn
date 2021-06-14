import bisect
import numpy
from .geometry3d.graphicsPrimitives import Cylinder, Cone
from neuron import h

# TODO: remove indirection (e.g. use _h_x3d instead of h.x3d)


def centroids_by_segment(sec):
    """
    given a section, returns a dictionary whose entries are lists of cylinders
    of radius 0 that should be used for distance calculations, keyed by section

    .. warning::

        Does not currently support non-frustum based sections (i.e. no support
        for new 3d styles, like soma outlines)

    .. warning::

        This assumes a 3d style exists. The safest way to call this is to call
        h.define_shape() first
    """
    # TODO: fix the issue described in the warning
    #       (when this was written, these objects were only under development)

    n3d = sec.n3d()
    length = sec.L

    arc3d = [sec.arc3d(i) for i in range(n3d)]
    x3d = numpy.array([sec.x3d(i) for i in range(n3d)])
    y3d = numpy.array([sec.y3d(i) for i in range(n3d)])
    z3d = numpy.array([sec.z3d(i) for i in range(n3d)])
    diam3d = numpy.array([sec.diam3d(i) for i in range(n3d)])

    dx = length / sec.nseg
    objs = {}
    for i in range(sec.nseg):
        x_lo = i * dx
        x_hi = (i + 1) * dx
        pts = [x_lo] + _values_strictly_between(x_lo, x_hi, arc3d) + [x_hi]
        local_x3d = numpy.interp(pts, arc3d, x3d)
        local_y3d = numpy.interp(pts, arc3d, y3d)
        local_z3d = numpy.interp(pts, arc3d, z3d)
        local_diam3d = numpy.interp(pts, arc3d, diam3d)
        local_objs = []
        for j in range(len(pts) - 1):
            x0, y0, z0, r0 = (
                local_x3d[j],
                local_y3d[j],
                local_z3d[j],
                local_diam3d[j] / 2.0,
            )
            x1, y1, z1, r1 = (
                local_x3d[j + 1],
                local_y3d[j + 1],
                local_z3d[j + 1],
                local_diam3d[j + 1] / 2.0,
            )
            if x0 != x1 or y0 != y1 or z0 != z1:
                local_objs.append(Cylinder(x0, y0, z0, x1, y1, z1, 0))
        objs[sec((i + 0.5) / sec.nseg)] = local_objs
    return objs


def objects_by_segment(sec):
    """
    given a section, returns a dictionary whose entries are lists of objects
    that should be used for distance calculations, keyed by section

    .. warning::

        Does not currently support non-frustum based sections (i.e. no support
        for new 3d styles, like soma outlines)

    .. warning::

        This assumes a 3d style exists. The safest way to call this is to call
        h.define_shape() first
    """
    # TODO: fix the issue described in the warning
    #       (when this was written, these objects were only under development)

    n3d = sec.n3d()
    length = sec.L

    arc3d = [sec.arc3d(i) for i in range(n3d)]
    x3d = numpy.array([sec.x3d(i) for i in range(n3d)])
    y3d = numpy.array([sec.y3d(i) for i in range(n3d)])
    z3d = numpy.array([sec.z3d(i) for i in range(n3d)])
    diam3d = numpy.array([sec.diam3d(i) for i in range(n3d)])

    dx = length / sec.nseg
    objs = {}
    for i in range(sec.nseg):
        x_lo = i * dx
        x_hi = (i + 1) * dx
        pts = [x_lo] + _values_strictly_between(x_lo, x_hi, arc3d) + [x_hi]
        local_x3d = numpy.interp(pts, arc3d, x3d)
        local_y3d = numpy.interp(pts, arc3d, y3d)
        local_z3d = numpy.interp(pts, arc3d, z3d)
        local_diam3d = numpy.interp(pts, arc3d, diam3d)
        local_objs = []
        for j in range(len(pts) - 1):
            x0, y0, z0, r0 = (
                local_x3d[j],
                local_y3d[j],
                local_z3d[j],
                local_diam3d[j] / 2.0,
            )
            x1, y1, z1, r1 = (
                local_x3d[j + 1],
                local_y3d[j + 1],
                local_z3d[j + 1],
                local_diam3d[j + 1] / 2.0,
            )
            if r0 == r1:
                local_objs.append(Cylinder(x0, y0, z0, x1, y1, z1, r0))
            else:
                local_objs.append(Cone(x0, y0, z0, r0, x1, y1, z1, r1))
        objs[sec((i + 0.5) / sec.nseg)] = local_objs
    return objs


def _values_between(lo, hi, data):
    i_lo = bisect.bisect_left(data, lo)
    i_hi = bisect.bisect_right(data, hi)
    return data[i_lo:i_hi]


def _values_strictly_between(lo, hi, data):
    temp = _values_between(lo, hi, data)
    if temp and temp[0] == lo:
        temp = temp[1:]
    if temp and temp[-1] == hi:
        temp = temp[:-1]
    return temp

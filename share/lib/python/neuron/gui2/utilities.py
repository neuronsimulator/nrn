import numpy
import bisect


def _segment_3d_pts(sec):
    """
    given a section, return a list of (xs, ys, zs, diams) for each segment

    .. warning::

        Does not currently support non-frustum based sections (i.e. no support
        for new 3d styles, like soma outlines)

    .. warning::

        This assumes a 3d style exists. The safest way to call this is to call
        h.define_shape() first
    """
    # TODO: fix the issue described in the warning
    #       (when this was written, these objects were only under development)

    # this is based on neuron.rxd.dimension3.centroids_by_segment(sec)

    n3d = sec.n3d()
    length = sec.L

    arc3d = [sec.arc3d(i) for i in range(n3d)]
    x3d = numpy.array([sec.x3d(i) for i in range(n3d)])
    y3d = numpy.array([sec.y3d(i) for i in range(n3d)])
    z3d = numpy.array([sec.z3d(i) for i in range(n3d)])
    diam3d = numpy.array([sec.diam3d(i) for i in range(n3d)])

    dx = length / sec.nseg
    result = []
    for i in range(sec.nseg):
        x_lo = i * dx
        x_hi = (i + 1) * dx
        pts = [x_lo] + _values_strictly_between(x_lo, x_hi, arc3d) + [x_hi]
        local_x3d = list(numpy.interp(pts, arc3d, x3d))
        local_y3d = list(numpy.interp(pts, arc3d, y3d))
        local_z3d = list(numpy.interp(pts, arc3d, z3d))
        local_diam3d = list(numpy.interp(pts, arc3d, diam3d))
        result.append((local_x3d, local_y3d, local_z3d, local_diam3d, pts))
    return result


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

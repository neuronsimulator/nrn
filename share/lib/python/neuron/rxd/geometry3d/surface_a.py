import ctypes
import numpy
from .. import options
from neuron import nrn_dll_sym
from ctypes import c_double, POINTER


"""Function to take any surface vox with information on distance of each vertex from surface 
and output approx. surface area contained in that vox"""


def surface_area(itemlist, vox, grid):
    res = options.ics_partial_surface_resolution
    i0, j0, k0 = vox
    rng = range(res + 1)
    rng0 = range(res)
    dx, dy, dz = grid["dx"], grid["dy"], grid["dz"]
    Sx, Sy, Sz = dx / res, dy / res, dz / res
    x0 = grid["xlo"] + i0 * dx
    y0 = grid["ylo"] + j0 * dy
    z0 = grid["zlo"] + k0 * dz
    dist = {}
    for i in rng:
        x = x0 + i * Sx
        for j in rng:
            y = y0 + j * Sy
            for k in rng:
                z = z0 + k * Sz
                dist[(i, j, k)] = min([f.distance(x, y, z) for f in itemlist])
    area = 0
    for i in rng0:
        for j in rng0:
            for k in rng0:
                area += sub_surface_area(
                    dist[i, j, k],
                    dist[i + 1, j, k],
                    dist[i + 1, j + 1, k],
                    dist[i, j + 1, k],
                    dist[i, j, k + 1],
                    dist[i + 1, j, k + 1],
                    dist[i + 1, j + 1, k + 1],
                    dist[i, j + 1, k + 1],
                    0,
                    Sx,
                    0,
                    Sy,
                    0,
                    Sz,
                )
    return area


def sub_surface_area(v0, v1, v2, v3, v4, v5, v6, v7, x0, x1, y0, y1, z0, z1):
    def allornone(lst):
        return all(lst) or not any(lst)

    # if all corners are inside -- return 0
    if allornone(
        [x <= options.ics_distance_threshold for x in [v0, v1, v2, v3, v4, v5, v6, v7]]
    ):
        return 0

    results = numpy.array([0.0] * 48)

    find_triangles = nrn_dll_sym("find_triangles")
    find_triangles.argtypes = [
        c_double,
        c_double,
        c_double,
        c_double,
        c_double,
        c_double,
        c_double,
        c_double,
        c_double,  # value0 - value7
        c_double,
        c_double,
        c_double,
        c_double,
        c_double,
        c_double,  # x0, x1, y0, y1, z0, z1
        POINTER(c_double),
    ]

    llgramarea = nrn_dll_sym("llgramarea")
    llgramarea.argtypes = [POINTER(c_double), POINTER(c_double), POINTER(c_double)]
    llgramarea.restype = c_double

    results_ptr = results.ctypes.data_as(ctypes.POINTER(ctypes.c_double))

    count = find_triangles(
        abs(options.ics_distance_threshold),
        v0,
        v1,
        v2,
        v3,
        v4,
        v5,
        v6,
        v7,
        x0,
        x1,
        y0,
        y1,
        z0,
        z1,
        results_ptr,
    )

    pt0 = numpy.array([0.0] * 3)
    pt1 = numpy.array([0.0] * 3)
    pt2 = numpy.array([0.0] * 3)

    pt0_ptr = pt0.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    pt1_ptr = pt1.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    pt2_ptr = pt2.ctypes.data_as(ctypes.POINTER(ctypes.c_double))

    results = results.reshape(16, 3)

    tri_i = 0
    area = 0
    for tri_i in range(count):
        pt0[:] = results[0 + 3 * tri_i]
        pt1[:] = results[1 + 3 * tri_i]
        pt2[:] = results[2 + 3 * tri_i]

        area += llgramarea(pt0_ptr, pt1_ptr, pt2_ptr) / 2.0

    return area

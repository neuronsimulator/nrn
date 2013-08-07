import numpy
import graphicsPrimitives
import neuron
import numpy
from numpy import sqrt, fabs
import itertools
import ctypes

#
# connect to dll via ctypes
#
nrn_dll_sym = neuron.nrn_dll_sym
    
#
# declare prototypes
#


# int find_triangles(double value0, double value1, double value2, double value3, double value4, double value5, double value6, double value7, double x0, double x1, double y0, double y1, double z0, double z1, double* out)
find_triangles = nrn_dll_sym('geometry3d_find_triangles')
find_triangles.argtypes = [ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double,
                           ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double,
                           ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double,
                           ctypes.c_double, ctypes.c_double,
                           numpy.ctypeslib.ndpointer(dtype=numpy.float64, ndim=1, flags='C_CONTIGUOUS'), ctypes.c_int]

# int geometry3d_process_cell(int i, int j, int k, PyObject* objects, double* xs, double* ys, double* zs, double* tridata, int start);
# process_cell = nrn_dll_sym('geometry3d_process_cell')
# process_cell.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.py_object,
#                          numpy.ctypeslib.ndpointer(dtype=numpy.float64, ndim=1, flags='C_CONTIGUOUS'),
#                          numpy.ctypeslib.ndpointer(dtype=numpy.float64, ndim=1, flags='C_CONTIGUOUS'),
#                          numpy.ctypeslib.ndpointer(dtype=numpy.float64, ndim=1, flags='C_CONTIGUOUS'),
#                          numpy.ctypeslib.ndpointer(dtype=numpy.float64, ndim=1, flags='C_CONTIGUOUS'),
#                          ctypes.c_int]



# for performance measures only
import time



max_chunks = 10000000

total_surface_tests = 0
corner_tests = 0

def contains_surface(i, j, k, objdist, xs, ys, zs, dx, r_inner, r_outer):
    has_neg = False
    has_pos = False
    # def double x, y, z
    
    global total_surface_tests, corner_tests
    total_surface_tests += 1
    
    
    
    # sphere tests added 2012-12-10
    xbar = xs[i] + dx / 2.
    ybar = ys[j] + dx / 2.
    zbar = zs[k] + dx / 2.
    
    d = fabs(objdist(xbar, ybar, zbar))
    if d <= r_inner: return True
    if d >= r_outer: return False
    
    
    
    # indeterminant from spheres; check corners
    corner_tests += 1
    #print 'corner_tests = %d out of total_surface_tests = %d' % (corner_tests, total_surface_tests)
    for x in xs[i : i + 2]:
        for y in ys[j : j + 2]:
            for z in zs[k : k + 2]:
                d = objdist(x, y, z)
                if d <= 0:
                    has_neg = True
                if d >= 0:
                    has_pos = True
                if has_pos and has_neg:
                    return True
    return False

def process_cell(i, j, k, objects, xs, ys, zs, tridata, start):
    # def double x, y, z, x1, y1, z1
    # def tuple position
    x, y, z = xs[i], ys[j], zs[k]
    x1, y1, z1 = xs[i + 1], ys[j + 1], zs[k + 1]
    position = ((x, y, z),
                (x1, y, z),
                (x1, y1, z),
                (x, y1, z),
                (x, y, z1),
                (x1, y, z1),
                (x1, y1, z1),
                (x, y1, z1))
    # def double value0, value1, value2, value3, value4, value5, value6, value7
    value0, value1, value2, value3, value4, value5, value6, value7 = [min([obj.distance(*p) for obj in objects]) for p in position]
    if any(numpy.isnan([value0, value1, value2, value3, value4, value5, value6, value7])): print 'bad distance'
    return start + 9 * find_triangles(value0, value1, value2, value3, value4, value5, value6, value7, x, x1, y, y1, z, z1, tridata, start)

# def list _deltas

_deltas = [[-1, -1, -1], [-1, -1, 0], [-1, -1, 1], [-1, 0, -1], [-1, 0, 0],
           [-1, 0, 1], [-1, 1, -1], [-1, 1, 0], [-1, 1, 1], [0, -1, -1],
           [0, -1, 0], [0, -1, 1], [0, 0, -1], [0, 0, 1], [0, 1, -1], [0, 1, 0],
           [0, 1, 1], [1, -1, -1], [1, -1, 0], [1, -1, 1], [1, 0, -1],
           [1, 0, 0], [1, 0, 1], [1, 1, -1], [1, 1, 0], [1, 1, 1]]


# This assumes dx = dy = dz
# CTNG:constructivecubes
def triangulate_surface(objects, xs, ys, zs, internal_membranes):
    #def int i, j, k, di, dj, dk
    #def double area
    grid_dx = xs[1] - xs[0]
    grid_dy = ys[1] - ys[0]
    
    grid_dz = zs[1] - zs[0]
    # TODO: we assume a cubic mesh
    # assert(grid_dx == grid_dy == grid_dz)
    r_inner = grid_dx / 2.
    r_outer = r_inner * sqrt(3)
    
    start = time.time()
    
    #def list cell_list

    to_process = {}
    
    cell_count = 0
    dup_count = 0
    
    cell_list2 = []
    
    #def numpy.ndarray[numpy.float_t, ndim=1] triangles
    surf_count = 0
    
    #def dict cur_processed
    brute_force_count = 0

    #def int count, numcompartments
    # locate all the potential boundary locations
    triangles = numpy.zeros(1000)
    triangles_i = 0
    for count, obj in enumerate(objects):
        objdist = obj.distance
        cell_list = []
        cur_processed = {}
        for i, j, k in obj.starting_points(xs, ys, zs):
            for di, dj, dk in itertools.product([0, -1, 1, -2, 2], [0, -1, 1, -2, 2], [0, -1, 1, -2, 2]):
                if contains_surface(i + di, j + dj, k + dk, objdist, xs, ys, zs, grid_dx, r_inner, r_outer):
                    cell_list.append((i + di, j + dj, k + dk))
                    break
            if cell_list: break
        else:
            # CTNG:systemsearch
            brute_force_count += 1

            numcompartments = (4 + len(range(bisect.bisect_left(xs, obj.xlo), bisect.bisect_right(xs, obj.xhi)))) * (4 + len(range(bisect.bisect_left(ys, obj.ylo), bisect.bisect_right(ys, obj.yhi)))) * (4 + len(range(bisect.bisect_left(zs, obj.zlo), bisect.bisect_right(zs, obj.zhi))))
            for i, j, k in itertools.product(xrange(bisect.bisect_left(xs, obj.xlo) - 2, bisect.bisect_right(xs, obj.xhi) + 2), xrange(bisect.bisect_left(ys, obj.ylo) - 2, bisect.bisect_right(ys, obj.yhi) + 2), xrange(bisect.bisect_left(zs, obj.zlo) - 2, bisect.bisect_right(zs, obj.zhi) + 2)):
                if contains_surface(i, j, k, objdist, xs, ys, zs, grid_dx, r_inner, r_outer):
                    cell_list.append((i, j, k))
                    break
            else:
                #print 'objects with no contribution to surface'
                pass

        if not internal_membranes:
            # CTNG:floodfill
            while cell_list:
                cell_id = cell_list.pop()
                if cell_id not in cur_processed:
                    cur_processed[cell_id] = 0
                    i, j, k = cell_id
                    if contains_surface(i, j, k, objdist, xs, ys, zs, grid_dx, r_inner, r_outer):
                        to_process[cell_id] = 0
                        for di, dj, dk in _deltas:
                            cell_list.append((i + di, j + dj, k + dk))
        else:
            while cell_list:
                cell_id = cell_list.pop()
                if cell_id not in cur_processed:
                    cur_processed[cell_id] = 0
                    i, j, k = cell_id
                    if contains_surface(i, j, k, objdist, xs, ys, zs, grid_dx, r_inner, r_outer):
                        if triangles_i > triangles.size - 50:
                            triangles.resize(triangles.size * 2, refcheck=False)
                        triangles_i = process_cell(i, j, k, [obj], xs, ys, zs, triangles, triangles_i)
                        for di, dj, dk in _deltas:
                            cell_list.append((i + di, j + dj, k + dk))
    
    if internal_membranes:
        triangles.resize(triangles_i, refcheck=False)
        return triangles
    
    cur_processed = None
    
    # use chunks no smaller than 10 voxels across, but aim for max_chunks chunks
    chunk_size = max(10, int((len(xs) * len(ys) * len(zs) / max_chunks) ** (1 / 3.)))
    almost = chunk_size - 1
    nx = (len(xs) + almost) // chunk_size
    ny = (len(ys) + almost) // chunk_size
    nz = (len(zs) + almost) // chunk_size

    dx = xs[1] - xs[0]
    # this is bigger than sqrt(3) * dx / 2 \approx 0.866, the distance from center of cube to corner
    max_d = dx * .87 * chunk_size
    
    # safety factor
    max_d *= 2

    chunk_objs = [[[[] for k in xrange(nz)] for j in xrange(ny)] for i in xrange(nx)]
    chunk_pts = [[[[] for k in xrange(nz)] for j in xrange(ny)] for i in xrange(nx)]
    
    # identify chunk_objs
    lenx = len(xs)
    leny = len(ys)
    lenz = len(zs)
    for obj in objects:
        for i in xrange(nx):
            xlo, xhi = xs[i * chunk_size], xs[min((i + 1) * chunk_size - 1, lenx - 1)]
            chunk_objsi = chunk_objs[i]
            # CTNG:testbb
            if not obj.overlaps_x(xlo, xhi): continue
            for j in xrange(ny):
                ylo, yhi = ys[j * chunk_size], ys[min(leny - 1, (j + 1) * chunk_size - 1)]
                if not obj.overlaps_y(ylo, yhi): continue
                for k in xrange(nz):
                    zlo, zhi = zs[k * chunk_size], zs[min(lenz - 1, (k + 1) * chunk_size - 1)]
                    if not obj.overlaps_z(zlo, zhi): continue
                    # CTNG:testball
                    if obj.distance((xlo + xhi) * .5, (ylo + yhi) * .5, (zlo + zhi) * .5) < max_d:
                        chunk_objsi[j][k].append(obj)
                

    # identify chunk_pts
    for i, j, k in to_process.keys():
        chunk_pts[i // chunk_size][j // chunk_size][k // chunk_size].append((i, j, k))

    num_keys = len(to_process.keys())
    missing_objs = 0
    to_process = None

    triangles = numpy.zeros(45 * num_keys)
    starti = 0
    # handle a chunk at a time
    for a in xrange(nx):
        chunk_objsa = chunk_objs[a]
        chunk_ptsa = chunk_pts[a]
        for b in xrange(ny):
            for c in xrange(nz):
                objs = chunk_objsa[b][c]
                cells = chunk_ptsa[b][c]
                if cells and not objs:
                    # we should never get here; this is just a sanity check
                    missing_objs += 1
                    continue
                for i, j, k in cells:
                    starti = process_cell(i, j, k, objs, xs, ys, zs, triangles, starti)
    # None of the following reduces process memory (although HAVE to do the triangles.resize)
    triangles.resize(starti, refcheck=False)
    
    return triangles


# CTNG:surfacearea
def tri_area(triangles):
    return geometry3d_sum_area_of_triangles(triangles, len(triangles))


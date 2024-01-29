import os
import numpy
import graphicsPrimitives
import neuron
import numpy
cimport numpy
import itertools
import bisect
cimport cython


"""
The surfaces module
"""

cdef extern int find_triangles(double value0, double value1, double value2, double value3, double value4, double value5, double value6, double value7, double x0, double x1, double y0, double y1, double z0, double z1, double* out)
cdef extern double llgramarea(double* p0, double* p1, double* p2)
cdef extern double llpipedfromoriginvolume(double* p0, double* p1, double* p2)

cdef extern from "math.h":
    double sqrt(double)
    double fabs(double)



def _register_on_neighbor_map(the_map, pt, neighbor):
    # does not assume neighbor relations are bidirectional
    if pt in the_map:
        the_map[pt].append(neighbor)
    else:
        the_map[pt] = [neighbor]


max_chunks = 10000000

cdef int total_surface_tests = 0
cdef int corner_tests = 0

@cython.boundscheck(False)
@cython.wraparound(False)
#cdef bint contains_surface(int i, int j, int k, objdist, numpy.ndarray[numpy.float_t, ndim=1] xs, numpy.ndarray[numpy.float_t, ndim=1] ys, numpy.ndarray[numpy.float_t, ndim=1] zs, double dx, double r_inner, double r_outer, bint reject_if_outside, bint print_reject_reason=False):
def contains_surface(i, j, k, objdist, xs, ys, zs, dx, r_inner, r_outer, reject_if_outside, bint print_reject_reason=False):
    cdef bint has_neg = False
    cdef bint has_pos = False
    cdef double x, y, z

    global total_surface_tests, corner_tests
    total_surface_tests += 1

    # sphere tests added 2012-12-10
    cdef double xbar = xs[i] + dx / 2.
    cdef double ybar = ys[j] + dx / 2.
    cdef double zbar = zs[k] + dx / 2.

    d = fabs(objdist(xbar, ybar, zbar))
    if d <= r_inner: return True
    if d >= r_outer and reject_if_outside:
        if print_reject_reason:
            print('would normally reject because at (%g, %g, %g): d = %g, r_outer = %g     (dx = %g)' % (xbar, ybar, zbar, d, r_outer, dx))
        else:
            return False



    # indeterminant from spheres; check corners
    corner_tests += 1
    #print 'corner_tests = %d out of total_surface_tests = %d' % (corner_tests, total_surface_tests)
    for x in xs[i : i + 2]:
        for y in ys[j : j + 2]:
            for z in zs[k : k + 2]:
                d = objdist(x, y, z)
                if print_reject_reason:
                    print('at (%g, %g, %g): d = %g' % (x, y, z, d))
                if d <= 0:
                    has_neg = True
                if d >= 0:
                    has_pos = True
                if has_pos and has_neg:
                    return True

    return False



@cython.boundscheck(False)
@cython.wraparound(False)
#cdef int process_cell(int i, int j, int k, list objects, numpy.ndarray[numpy.float_t, ndim=1] xs, numpy.ndarray[numpy.float_t, ndim=1] ys, numpy.ndarray[numpy.float_t, ndim=1] zs, numpy.ndarray[numpy.float_t, ndim=1] tridata, int start, bint print_values=False):
def process_cell(int i, int j, int k, list objects, numpy.ndarray[numpy.float_t, ndim=1] xs, numpy.ndarray[numpy.float_t, ndim=1] ys, numpy.ndarray[numpy.float_t, ndim=1] zs, numpy.ndarray[numpy.float_t, ndim=1] tridata, int start, bint store_areas=False, numpy.ndarray[numpy.float_t, ndim=3] areas=None, bint print_values=False):
    cdef int new_index
    cdef double x, y, z, x1, y1, z1
    cdef list position
    x, y, z = xs[i], ys[j], zs[k]
    x1, y1, z1 = xs[i + 1], ys[j + 1], zs[k + 1]

    position = [(x, y, z),
                (x1, y, z),
                (x1, y1, z),
                (x, y1, z),
                (x, y, z1),
                (x1, y, z1),
                (x1, y1, z1),
                (x, y1, z1)]
    cdef double value0, value1, value2, value3, value4, value5, value6, value7
    value0, value1, value2, value3, value4, value5, value6, value7 = [min([objdist(*p) for objdist in objects]) for p in position]
    if print_values:
        print('(x, y, z) = (%7f, %7f, %7f); (x1, y1, z1) = (%7f, %7f, %7f)' % (x, y, z, x1, y1, z1))
        print('%7f %7f %7f %7f %7f %7f %7f %7f' % (value0, value1, value2, value3, value4, value5, value6, value7))
        print('last obj distance to position[4]: ', objects[len(objects)-1](*position[4]))
        print('distance to position[4] with everything but the last object:', min([objdist(*position[4]) for objdist in objects[:len(objects)-1]]))
        print('distance to position[4] with everything:', min([objdist(*position[4]) for objdist in objects[:]]))
        print('last object:', objects[len(objects) - 1])
        print('position[4]:', position[4])

    new_index = start + 9 * find_triangles(value0, value1, value2, value3, value4, value5, value6, value7, x, x1, y, y1, z, z1, &tridata[start])
    if store_areas:
        areas[i, j, k] = _tri_area(tridata, start, new_index)
    return new_index

# a matrix whose values are unambiguously outside the object
big_number_matrix = numpy.zeros([4, 4, 4])
for i in range(4):
    for j in range(4):
        for k in range(4):
            big_number_matrix[i, j, k] = 9999 + i * 16 + j * 4 + k

@cython.boundscheck(False)
@cython.wraparound(False)
def volume_inside_cell(int i, int j, int k, list objects, numpy.ndarray[numpy.float_t, ndim=1] xs, numpy.ndarray[numpy.float_t, ndim=1] ys, numpy.ndarray[numpy.float_t, ndim=1] zs):
    cdef double x0 = xs[i], y0 = ys[j], z0 = zs[k]
    cdef double x1 = xs[i + 1], y1 = ys[j + 1], z1 = zs[k + 1]
    cdef double dx = x1 - x0, dy = y1 - y0, dz = z1 - z0
    cdef numpy.ndarray[numpy.float_t, ndim=1] local_xs = numpy.array([x0 - dx, x0, x1, x0 + 2 * dx])
    cdef numpy.ndarray[numpy.float_t, ndim=1] local_ys = numpy.array([y0 - dy, y0, y1, y0 + 2 * dy])
    cdef numpy.ndarray[numpy.float_t, ndim=1] local_zs = numpy.array([z0 - dz, z0, z1, z0 + 2 * dz])
    # big numbers unambiguously outside, no two of which are the same so (probably) no risk of dividing by zero?
    cdef numpy.ndarray[numpy.float_t, ndim=3] data = numpy.array(big_number_matrix)
    cdef double x, y, z
    cdef int i1, j1, k1
    if not objects:
        print('grr... it thinks there is surface when no nearby objects.')
    for i, x in zip([1, 2], [x0, x1]):
        for j, y in zip([1, 2], [y0, y1]):
            for k, z in zip([1, 2], [z0, z1]):
                data[i, j, k] = min([objdist(x, y, z) for objdist in objects])

    # TODO: I probably don't need the 8 corner voxels
    # 9 doubles/tri * 5 tri/cell * 27 cells = 1215 doubles
    cdef numpy.ndarray[numpy.float_t, ndim=1] tridata = numpy.zeros(1215)
    cdef int start = 0
    for i in range(3):
        i1 = i + 1
        for j in range(3):
            j1 = j + 1
            for k in range(3):
                k1 = k + 1
                x, y, z = xs[i], ys[j], zs[k]
                x1, y1, z1 = xs[i + 1], ys[j + 1], zs[k + 1]
                value0, value1, value2, value3, value4, value5, value6, value7 = (data[i, j, k],
                                                                                  data[i1, j, k],
                                                                                  data[i1, j1, k],
                                                                                  data[i, j1, k],
                                                                                  data[i, j, k1],
                                                                                  data[i1, j, k1],
                                                                                  data[i1, j1, k1],
                                                                                  data[i, j1, k1])
                # TODO: if i = 0, j = k = 1, then this finds the triangles to the left (so can get area that way), etc...
                start += 9 * find_triangles(value0, value1, value2, value3, value4, value5, value6, value7, x, x1, y, y1, z, z1, &tridata[start])
    # at this point tridata contains a triangularization that encloses the entire cell's volume
    tridata.resize(start, refcheck=False)
    return tri_volume(tridata)


cdef append_with_deltas(list cell_list, int i, int j, int k):
    cdef int im1 = i - 1, jm1 = j - 1, km1 = k - 1, ip1 = i + 1, jp1 = j + 1, kp1 = k + 1
    cell_list += [(im1, jm1, km1), (im1, jm1, k), (im1, jm1, kp1), (im1, j, km1),
                  (im1, j, k), (im1, j, kp1), (im1, jp1, km1), (im1, jp1, k),
                  (im1, jp1, kp1), (i, jm1, km1), (i, jm1, k), (i, jm1, kp1),
                  (i, j, km1), (i, j, kp1), (i, jp1, km1), (i, jp1, k), (i, jp1, kp1),
                  (ip1, jm1, km1), (ip1, jm1, k), (ip1, jm1, kp1), (ip1, j, km1),
                  (ip1, j, k), (ip1, j, kp1), (ip1, jp1, km1), (ip1, jp1, k),
                  (ip1, jp1, kp1)]









# TODO: move this someplace else. also useful for voxelize
@cython.boundscheck(False)
@cython.wraparound(False)
cpdef chunkify(objects, xs, ys, zs, int chunk_size, double dx):

    cdef int almost = chunk_size - 1
    cdef int nx = (len(xs) + almost) // chunk_size
    cdef int ny = (len(ys) + almost) // chunk_size
    cdef int nz = (len(zs) + almost) // chunk_size
    cdef list objects_distances = [obj.distance for obj in objects]
    cdef int m, i, j, k

    # this is bigger than sqrt(3) * dx / 2 \approx 0.866, the distance from center of cube to corner
    cdef double max_d = dx * .87 * chunk_size

    # safety factor
    max_d *= 2

    cdef list chunk_objs = [[[[] for k in range(nz)] for j in range(ny)] for i in range(nx)]

    # identify chunk_objs
    cdef int lenx = len(xs)
    cdef int leny = len(ys)
    cdef int lenz = len(zs), robj = 0
    cdef double xlo, xhi, ylo, yhi, zlo, zhi
    cdef double bufferdx = 3 * dx

    # TODO: the is_skew_cone business is here because distances are not the real distances in that case;
    #       remove it when I fix this (and will get better performance)
    for m, obj in enumerate(objects):
        is_skew_cone = isinstance(obj, graphicsPrimitives.SkewCone)
        objdist = obj.distance
        for i in range(nx):
            xlo, xhi = xs[i * chunk_size], xs[min((i + 1) * chunk_size - 1, lenx - 1)]
            chunk_objsi = chunk_objs[i]
            # CTNG:testbb
            if not obj.overlaps_x(xlo - bufferdx, xhi + bufferdx): continue
            for j in range(ny):
                ylo, yhi = ys[j * chunk_size], ys[min(leny - 1, (j + 1) * chunk_size - 1)]
                if not obj.overlaps_y(ylo - bufferdx, yhi + bufferdx): continue
                for k in range(nz):
                    zlo, zhi = zs[k * chunk_size], zs[min(lenz - 1, (k + 1) * chunk_size - 1)]
                    if not obj.overlaps_z(zlo - bufferdx, zhi + bufferdx): continue
                    # CTNG:testball
                    if is_skew_cone or objdist((xlo + xhi) * .5, (ylo + yhi) * .5, (zlo + zhi) * .5) < max_d:
                        chunk_objsi[j][k].append(objdist)
    return chunk_objs, nx, ny, nz










# This assumes dx = dy = dz
# CTNG:constructivecubes
@cython.boundscheck(False)
@cython.wraparound(False)
cpdef triangulate_surface(list objects, xs, ys, zs, internal_membranes):
    # use chunks no smaller than 10 voxels across, but aim for max_chunks chunks
    cdef int chunk_size = max(10, int((len(xs) * len(ys) * len(zs) / max_chunks) ** (1 / 3.)))
    cdef double grid_dx = xs[1] - xs[0], grid_dy = ys[1] - ys[0], grid_dz = zs[1] - zs[0]
    cdef double dx = grid_dx

    cdef list chunk_objs
    cdef int nx, ny, nz
    chunk_objs, nx, ny, nz = chunkify(objects, xs, ys, zs, chunk_size, dx)
    return _triangulate_surface_given_chunks(objects, xs, ys, zs, internal_membranes, chunk_size, chunk_objs, nx, ny, nz, False, None)

cpdef _triangulate_surface_given_chunks(list objects, xs, ys, zs, internal_membranes, int chunk_size, list chunk_objs, int nx, int ny, int nz, bint store_areas, areas):
    cdef int i, j, k, di, dj, dk
    cdef double area
    cdef double grid_dx = xs[1] - xs[0], grid_dy = ys[1] - ys[0], grid_dz = zs[1] - zs[0]
    # TODO: we assume a cubic mesh
    # assert(grid_dx == grid_dy == grid_dz)
    cdef double r_inner = grid_dx / 2., r_outer = r_inner * sqrt(3)
    cdef double dx = grid_dx

    cdef list cell_list

    cdef dict to_process = {}

    cdef int cell_count = 0
    cdef int dup_count = 0
    cdef int last_starti

    cdef list cell_list2 = []

    cdef numpy.ndarray[numpy.float_t, ndim=1] triangles
    cdef int surf_count = 0

    cdef dict cur_processed
    cdef int brute_force_count = 0
    cdef list clip_objs
    cdef tuple cell_id
    cdef int new_index

    cdef int numcompartments
    # locate all the potential boundary locations
    triangles = numpy.zeros(1000)
    cdef int triangles_i = 0
    cdef bint reject_if_outside
    for m, obj in enumerate(objects):
        # TODO: remove all the stuff about reject_if_outside when have true SkewCone distances
        reject_if_outside = not(isinstance(obj, graphicsPrimitives.SkewCone))
        objdist = obj.distance
        clip_objs = sum([clipper.primitives for clipper in obj.get_clip()], [])
        cell_list = []
        cur_processed = {}
        for i, j, k in obj.starting_points(xs, ys, zs):
            for di, dj, dk in itertools.product([0, -1, 1, -2, 2], [0, -1, 1, -2, 2], [0, -1, 1, -2, 2]):
                if contains_surface(i + di, j + dj, k + dk, objdist, xs, ys, zs, grid_dx, r_inner, r_outer, reject_if_outside):
                    cell_list.append((i + di, j + dj, k + dk))
                    break
            if cell_list: break
        else:
            # CTNG:systemsearch
            brute_force_count += 1
            # TODO: we are highly unlikely to ever reach this code (happened 0 times with the neuron I tested)
            #       can we prove we never get here?

            numcompartments = (4 + len(range(bisect.bisect_left(xs, obj.xlo), bisect.bisect_right(xs, obj.xhi)))) * (4 + len(range(bisect.bisect_left(ys, obj.ylo), bisect.bisect_right(ys, obj.yhi)))) * (4 + len(range(bisect.bisect_left(zs, obj.zlo), bisect.bisect_right(zs, obj.zhi))))
            for i, j, k in itertools.product(xrange(bisect.bisect_left(xs, obj.xlo) - 2, bisect.bisect_right(xs, obj.xhi) + 2), xrange(bisect.bisect_left(ys, obj.ylo) - 2, bisect.bisect_right(ys, obj.yhi) + 2), xrange(bisect.bisect_left(zs, obj.zlo) - 2, bisect.bisect_right(zs, obj.zhi) + 2)):
                if contains_surface(i, j, k, objdist, xs, ys, zs, grid_dx, r_inner, r_outer, reject_if_outside):
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
                    if contains_surface(i, j, k, objdist, xs, ys, zs, grid_dx, r_inner, r_outer, reject_if_outside):
                        to_process[cell_id] = 0
                        append_with_deltas(cell_list, i, j, k)

        else:
            # TODO: is this support ever really useful?
            while cell_list:
                cell_id = cell_list.pop()
                if cell_id not in cur_processed:
                    cur_processed[cell_id] = 0
                    i, j, k = cell_id
                    if contains_surface(i, j, k, objdist, xs, ys, zs, grid_dx, r_inner, r_outer, reject_if_outside):
                        if triangles_i > triangles.size - 50:
                            triangles.resize(triangles.size * 2, refcheck=False)
                        triangles_i = process_cell(i, j, k, [objdist], xs, ys, zs, triangles, triangles_i, store_areas=store_areas, areas=areas)
                        append_with_deltas(cell_list, i, j, k)

    if internal_membranes:
        triangles.resize(triangles_i, refcheck=False)
        return triangles

    cur_processed = None
    cdef list chunk_pts = [[[[] for k in range(nz)] for j in range(ny)] for i in range(nx)]




    # identify chunk_pts
    for i, j, k in to_process.keys():
        chunk_pts[i // chunk_size][j // chunk_size][k // chunk_size].append((i, j, k))

    cdef int num_keys = len(to_process.keys())
    cdef int missing_objs = 0

    triangles = numpy.zeros(45 * num_keys)
    cdef int starti = 0
    cdef int a, b, c
    cdef int extra_count
    cdef list obj_search
    # handle a chunk at a time
    for a in range(nx):
        chunk_objsa = chunk_objs[a]
        chunk_ptsa = chunk_pts[a]
        for b in range(ny):
            for c in range(nz):
                objs = chunk_objsa[b][c]
                cells = chunk_ptsa[b][c]
                if cells and not objs:
                    # we should never get here; this is just a sanity check
                    missing_objs += 1
                    continue
                for i, j, k in cells:
                    last_starti = starti
                    #starti = process_cell(i, j, k, objects_distances, xs, ys, zs, triangles, last_starti) #was objs
                    #tri_data = list(triangles[last_starti : starti])
                    start_i = process_cell(i, j, k, objs, xs, ys, zs, triangles, last_starti, store_areas=store_areas, areas=areas)
                    '''
                    # this an the two-above commented-out lines are for debugging to detect discrepancies between the
                    # chunked partitioning and using all the nodes
                    if starti - last_starti != len(tri_data) or any(tri_data != triangles[last_starti : starti]):
                        print('discrepancy in grid (%d, %d, %d) -- chunk_size = %d' % (i, j, k, chunk_size))
                        # identify which object(s)... grr...
                        """
                        for m in xrange(len(objects)):
                            tri_data2 = list(triangles[last_starti : starti])
                            starti = process_cell(i, j, k, objs + objects_distances[: m], xs, ys, zs, triangles, last_starti) #was objs
                            if starti - last_starti != len(tri_data2) or any(tri_data2 != triangles[last_starti : starti]):
                                print()'   missed object %d: %r' % (m, objects[m - 1]))
                            if starti - last_starti == len(tri_data) and all(tri_data == triangles[last_starti : starti]):
                                break
                        else:
                            print('    *** should never get here ***'""")
                    '''

    # some regions of the surface may yet be missing
    # it's possible for voxels to not contain detectable surface of any component, but
    # contain detectable surface for their union
    # to find, we look for holes in the surface and flood from them, checking against every object
    # TODO: could be smarter about this: at least check (enlarged) bounding boxes first


    last_starti = 0
    cdef dict process2
    pt_neighbor_map = {}
    cdef dict count
    cdef int old_start_i
    cdef list still_to_process, local_objs
    # append to the pt_neighbor_map
    process2 = {}
    count = {}

    for i in xrange(last_starti, starti, 9):
        pts = {}
        for j in xrange(3):
            pts[tuple(triangles[i + 3 * j : i + 3 * j + 3])] = 0
        pts = list(pts.keys())
        if len(pts) == 3:
            # only consider triangles of nonzero area
            for j in xrange(3):
                for k in xrange(3):
                    if j != k:
                        _register_on_neighbor_map(pt_neighbor_map, pts[j], pts[k])

    last_starti = starti
    xlo, ylo, zlo = xs[0], ys[0], zs[0]

    # if no holes, each point should have each neighbor listed more than once
    for pt, neighbor_list in zip(pt_neighbor_map.keys(), pt_neighbor_map.values()):
        count = {}
        for neighbor in neighbor_list:
            if neighbor not in count:
                count[neighbor] = 1
            else:
                count[neighbor] += 1
        for neighbor, ncount in zip(count.keys(), count.values()):
            if ncount <= 1:
                # Note: this assumes (as we stated above) that dx = dy = dz
                for point in (pt, neighbor):
                    i, j, k = (point[0] - xlo) // dx, (point[1] - ylo) // dx, (point[2] - zlo) // dx
                    for di in range(-1, 2):
                        for dj in range(-1, 2):
                            for dk in range(-1, 2):
                                cell_id = (i + di, j + dj, k + dk)
                                if cell_id not in process2 and cell_id not in to_process:
                                    process2[cell_id] = 0
                break

    still_to_process = list(process2.keys())
    #print 'len(still_to_process) = %d' % len(still_to_process)
    # flood on those still_to_process
    while still_to_process:
        cell_id = still_to_process.pop()
        # make sure we haven't already handled this voxel
        if cell_id not in to_process:
            i, j, k = cell_id
            old_start_i = starti
            local_objs = chunk_objs[i // chunk_size][j // chunk_size][k // chunk_size]
            if local_objs:
                for m, objdist in enumerate(local_objs):
                    if contains_surface(i, j, k, objdist, xs, ys, zs, dx, r_inner, r_outer, False):
                        print('item %d in grid(%d, %d, %d) contains previously undetected surface' % (m, i, j, k))
                        for n, obj in enumerate(objects):
                            if obj.distance == objdist:
                                print('    (i.e. global item %d: %r)' % (n, obj))
                                break
                #starti = process_cell(i, j, k, local_objs, xs, ys, zs, triangles, starti) # was objects_distances
                starti = process_cell(i, j, k, local_objs, xs, ys, zs, triangles, starti, store_areas=store_areas, areas=areas)

            # mark it off so we know we don't visit that grid point again
            to_process[cell_id] = 0
            if old_start_i != starti:
                append_with_deltas(still_to_process, i, j, k)

    triangles.resize(starti, refcheck=False)

    return triangles


cpdef double tri_area(numpy.ndarray[numpy.float_t, ndim=1] triangles):
    return _tri_area(triangles, 0, len(triangles))


# CTNG:surfacearea
@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double _tri_area(numpy.ndarray[numpy.float_t, ndim=1] triangles, int lo, int hi):
    cdef double doublearea = 0., local_area
    cdef int i
    for i in range(lo, hi, 9):
        local_area = llgramarea(&triangles[i], &triangles[3 + i], &triangles[6 + i])
        doublearea += local_area
        if numpy.isnan(local_area):
            print('tri_area exception: ', ', '.join([str(v) for v in triangles[i : i + 9]]))
    return doublearea * 0.5

@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double tri_volume(numpy.ndarray[numpy.float_t, ndim=1] triangles):
    cdef double sixtimesvolume = 0., local_vol
    cdef int i
    for i in range(0, len(triangles), 9):
        local_vol = llpipedfromoriginvolume(&triangles[i], &triangles[3 + i], &triangles[6 + i])
        sixtimesvolume += local_vol
        if numpy.isnan(local_vol):
            print('tri_volume exception:',)
            for j in range(i, i + 9):
                print(triangles[j]),
                
    return abs(sixtimesvolume / 6.)

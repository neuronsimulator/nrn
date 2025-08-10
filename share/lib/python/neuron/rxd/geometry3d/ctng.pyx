from neuron import h
import numpy
import itertools
cimport numpy
from numpy import linalg
cimport cython
from neuron.rxd.rxdException import RxDException
import neuron


"""
The ctng module
"""

cdef extern from "math.h":
    double sqrt(double)
    double fabs(double)


cdef tuple seg_line_intersection(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, bint clip):
    # returns None if parallel (so None if 0 or infinitely many intersections)
    # if clip is True, requires intersection on segment; else returns line-line intersection
    cdef double denom = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1)
    if denom == 0: return None
    cdef double u = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / denom
    if clip and not (0 <= u <= 1): return None
    return (x1 + u * (x2 - x1), y1 + u * (y2 - y1))

cdef tuple closest_pt(pt, list pts, z2):
    cdef double dist2 = float('inf')
    cdef object closest = None
    cdef double x0, y0
    cdef double d2, dx, dy, dz2
    x0 = pt[0]
    y0 = pt[1]
    dz2 = (pt[2] - z2) ** 2
    for p in pts:
        x, y = p
        dx = x0 - x
        dy = y0 - y
        d2 = dx * dx + dy * dy + dz2
        if d2 < dist2:
            dist2 = d2
            closest = p
    return tuple(closest)

cdef tuple extreme_pts(list pts):
    if len(pts) < 2:
        raise RxDException('extreme points computation failed')
    cdef double max_dist2 = -1
    cdef double d2, dx, dy, dz
    cdef tuple best_p1, best_p2
    cdef double x1, y1, z1
    cdef tuple pt2
    for i in range(len(pts)):
        x1, y1, z1 = pts[i]
        for j in range(i + 1, len(pts)):
            pt2 = pts[j]
            dx = x1 - pt2[0]
            dy = y1 - pt2[1]
            dz = z1 - pt2[2]
                d2 = dx * dx + dy * dy + dz * dz
                if d2 > max_dist2:
                    best_p1 = pts[i]
                    best_p2 = pt2
                    max_dist2 = d2
    return best_p1, best_p2

# helper function for maintaing the points-cones database
cdef void register(dict pts_cones_db, tuple pt, object cone):
    if pt not in pts_cones_db:
        pts_cones_db[pt] = []
    pts_cones_db[pt].append(cone)

# helper function that counts the number of points inside a region
@cython.boundscheck(False)
@cython.wraparound(False)
cdef int count_outside(region, list pts, double err):
    cdef numpy.ndarray[numpy.float_t, ndim=1] pt
    cdef int result = 0
    for pt in pts:
        if region.distance(pt[0], pt[1], pt[2]) > err:
            result += 1
    return result

@cython.boundscheck(False)
@cython.wraparound(False)
cdef tuple get_infinite_cones(double x0, double y0, double z0, double r0, double x1, double y1, double z1, double r1, double x2, double y2, double z2, double r2):
    from .graphicsPrimitives import Cone
    cdef double deltar, deltanr
    cdef numpy.ndarray[numpy.float_t, ndim=1] axis, naxis

    axis = numpy.array([x2 - x1, y2 - y1, z2 - z1])
    naxis = numpy.array([x1 - x0, y1 - y0, z1 - z0])
    deltar = r2 - r1; deltanr = r1 - r0
    deltar /= linalg.norm(axis); deltanr /= linalg.norm(naxis)
    axis /= linalg.norm(axis); naxis /= linalg.norm(naxis)
    #
    # sphere, clipped to extended cones
    # CTNG:trimsphere
    #
    offset0 = r0
    offset1 = r1
    offset1b = r1
    offset2 = r2
    if r0 - deltanr * offset0 < 0:
        # don't go that far (reduce offset0 so that this comes out 0)
        offset0 = r0 / deltanr
    if r1 + deltanr * offset1 < 0:
        offset1 = -r1 / deltanr
    if r1 - deltar * offset1b < 0:
        offset1b = r1 / deltar
    if r2 + deltar * offset2 < 0:
        offset2 = -r2 / deltar

    c0 = Cone(x0 - naxis[0] * offset0, y0 - naxis[1] * offset0, z0 - naxis[2] * offset0, r0 - deltanr * offset0, x1 + naxis[0] * offset1, y1 + naxis[1] * offset1, z1 + naxis[2] * offset1, r1 + deltanr * offset1)
    c1 = Cone(x1 - axis[0] * offset1b, y1 - axis[1] * offset1b, z1 - axis[2] * offset1b, r1 - deltar * offset1b, x2 + axis[0] * offset2, y2 + axis[1] * offset2, z2 + axis[2] * offset2, r2 + deltar * offset2)
    return c0, c1


@cython.boundscheck(False)
@cython.wraparound(False)
cdef list join_outside(double x0, double y0, double z0, double r0, double x1, double y1, double z1, double r1, double x2, double y2, double z2, double r2, double dx):
    from .graphicsPrimitives import Sphere, Intersection, Plane, Cone
    cdef numpy.ndarray[numpy.float_t, ndim=1] pt1, radial_vec, nradial_vec, axis, naxis

    c0, c1 = get_infinite_cones(x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2, r2)

    axis = numpy.array([x2 - x1, y2 - y1, z2 - z1])
    naxis = numpy.array([x1 - x0, y1 - y0, z1 - z0])
    axis /= linalg.norm(axis); naxis /= linalg.norm(naxis)
    #
    # sphere, clipped to extended cones
    # CTNG:trimsphere
    #
    sp = Sphere(x1, y1, z1, r1)
    sp.set_clip([Intersection([c0, c1])])

    result = [sp]
    # check to see if the clipped sphere covers the ends of the cones
    # if not, do something else :)

    # locate key vectors
    plane_normal = numpy.cross(axis, naxis)
    radial_vec = numpy.cross(plane_normal, axis)
    nradial_vec = numpy.cross(plane_normal, naxis)
    # normalize all of these
    radial_vec /= linalg.norm(radial_vec)
    nradial_vec /= linalg.norm(nradial_vec)

    # count the corners that are inside a sphere clipped to the cones
    pt1 = numpy.array([x1, y1, z1])
    cdef int left_corner_count = 2 - count_outside(sp, [pt1 + r1 * nradial_vec, pt1 - r1 * nradial_vec], dx * 0.5)
    cdef int corner_count = 2 - count_outside(sp, [pt1 + r1 * radial_vec, pt1 - r1 * radial_vec], dx * 0.5)
    #print('for join (%g, %g, %g; %g) - (%g, %g, %g; %g) - (%g, %g, %g; %g):' % (x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2, r2))
    #print('   left_corner_count = %g; corner_count = %g' % (left_corner_count, corner_count))
    if left_corner_count == corner_count == 2:
        sp.set_clip([Intersection([c0, c1,
            Plane(x1, y1, z1, axis[0], axis[1], axis[2]),
            Plane(x1, y1, z1, -naxis[0], -naxis[1], -naxis[2])])])
    elif left_corner_count < 2 and corner_count == 2:
        # clipping to c1 is too harsh, but c0 clip is fine
        sp.set_clip([Intersection([c0,
            Plane(x1, y1, z1, axis[0], axis[1], axis[2]),
            Plane(x1, y1, z1, -naxis[0], -naxis[1], -naxis[2])])])
    elif left_corner_count == 2 and corner_count < 2:
        # clipping to c0 is too harsh, but c1 clip is fine
        sp.set_clip([Intersection([c1,
            Plane(x1, y1, z1, axis[0], axis[1], axis[2]),
            Plane(x1, y1, z1, -naxis[0], -naxis[1], -naxis[2])])])
    elif left_corner_count < 2 and corner_count < 2:
        # both clips are too harsh; fall back to just using a sphere
        sp.set_clip([Intersection([
            Plane(x1, y1, z1, axis[0], axis[1], axis[2]),
            Plane(x1, y1, z1, -naxis[0], -naxis[1], -naxis[2])])])
    else:
        raise RxDException('unexpected corner_counts?')

    return result

@cython.wraparound(False)
@cython.boundscheck(False)
def soma_objects(x, y, z, sec, double x0, double y0, double z0, int n_soma_step):
    from .graphicsPrimitives import SkewCone
    cdef double diam1, diam2, somax, somay, somaz
    cdef list objects = []
    cdef list f_pts
    cdef dict seg_dict = {}
    
    somax, somay, somaz = x0, y0, z0

    xshifted = [xx - x0 for xx in x]
    yshifted = [yy - y0 for yy in y]
    # this is a hack to pretend everything is on the same z level
    zshifted = [0] * len(x)

    # locate the major and minor axis, adapted from import3d_gui.hoc
    m = h.Matrix(3, 3)
    for i, p in enumerate([xshifted, yshifted, zshifted]):
        for j, q in enumerate([xshifted, yshifted, zshifted]):
            if j < i: continue
            v = numpy.dot(p, q)
            m.setval(i, j, v)
            m.setval(j, i, v)
    # CTNG:majoraxis
    tobj = m.symmeig(m)
    # major axis is the one with largest eigenvalue
    major = m.getcol(tobj.max_ind())
    # minor is normal and in xy plane
    minor = m.getcol(3 - tobj.min_ind() - tobj.max_ind())
    #minor.x[2] = 0
    minor.div(minor.mag())

    x1 = x0; y1 = y0
    x2 = x1 + major.x[0]; y2 = y1 + major.x[1]

    xs_loop = x + [x[0]]
    ys_loop = y + [y[0]]

    # locate the extrema of the major axis CTNG:somaextrema
    # this is defined by the furthest points on it that lie on the minor axis
    pts = []
    pts_sources = {}
    for x3, y3 in zip(x, y):
        x4, y4 = x3 + minor.x[0], y3 + minor.x[1]
        pt = seg_line_intersection(x1, y1, x2, y2, x3, y3, x4, y4, False)
        if pt is not None:
            pts.append(pt)
            if pt not in pts_sources:
                pts_sources[pt] = []
            pts_sources[pt].append((x3, y3))

    major_p1, major_p2 = extreme_pts(pts)

    extreme1 = pts_sources[major_p1]
    extreme2 = pts_sources[major_p2]

    major_p1, major_p2 = numpy.array(major_p1), numpy.array(major_p2)
    del pts_sources

    if len(extreme1) != 1 or len(extreme2) != 1:
        raise RxDException('multiple most extreme points')
    extreme1 = extreme1[0]
    extreme2 = extreme2[0]
    major_length = linalg.norm(major_p1 - major_p2)
    delta_x, delta_y = major_p2 - major_p1
    delta_x /= n_soma_step
    delta_y /= n_soma_step

    f_pts = [extreme1]
    f_diams = [0]

    # CTNG:slicesoma
    for i in range(1, n_soma_step):
        x0, y0 = major_p1[0] + i * delta_x, major_p1[1] + i * delta_y
        # slice in dir of minor axis
        x1, y1 = x0 + minor.x[0], y0 + minor.x[1]
        pts = []
        for i in range(len(x)):
            pt = seg_line_intersection(xs_loop[i], ys_loop[i], xs_loop[i + 1], ys_loop[i + 1], x0, y0, x1, y1, True)
            if pt is not None: pts.append(pt)
        p1, p2 = extreme_pts(pts)
        p1, p2 = numpy.array(p1), numpy.array(p2)
        cx, cy = (p1 + p2) / 2.
        f_pts.append((cx, cy))
        f_diams.append(linalg.norm(p1 - p2))

    f_pts.append(extreme2)
    f_diams.append(0)

    segment_locs = [];        # location along soma for assigning segments
    path_length = 0;
    margin = 1/(sec.nseg*2)   # half a segment, normalized length
    for i in range(len(f_pts) - 1):
        pt1x, pt1y = f_pts[i]
        pt2x, pt2y = f_pts[i + 1]
        diam1 = f_diams[i]
        diam2 = f_diams[i + 1]
        thickness = numpy.sqrt((pt2x-pt1x)**2 + (pt2y-pt1y)**2)
        segment_locs.append(path_length+(thickness/2))
        path_length += thickness

        objects.append(SkewCone(pt1x, pt1y, z0, diam1 * 0.5, pt1x + delta_x, pt1y + delta_y, z0, diam2 * 0.5, pt2x, pt2y, z0))
    for j, s in enumerate(objects):
        loc = segment_locs[j]/path_length       # normalized along length
        for seg in sec:
            if numpy.abs(seg.x - loc) <= margin:
                seg_dict[s] = seg
                break;
    return seg_dict, f_pts

@cython.wraparound(False)
@cython.boundscheck(False)
cdef tuple _process_import3d_source(object source, int n_soma_step, dict soma_secs, dict soma_segment_dict):
    """Process Import3D type source."""
    cdef int no_parent_count = 0
    cdef list branches = []
    cdef list parent_sec_name = []
    
    cell = source
    # probably an Import3D type
    num_contours = sum(sec.iscontour_ for sec in cell.sections)
    if num_contours > 1:
        raise RxDException('more than one contour is not currently supported')
    if num_contours <= 1:
        # CTNG:soma
        for sec in cell.sections:
            if sec.iscontour_:
                x, y, z = [sec.raw.getrow(i).to_python() for i in range(3)]

                # compute the center of the contour based on uniformly spaced points around the perimeter
                center_vec = sec.contourcenter(sec.raw.getrow(0), sec.raw.getrow(1), sec.raw.getrow(2))
                x0, y0, z0 = [center_vec.x[i] for i in range(3)]
                new_objects, f_pts = soma_objects(x, y, z, sec, x0, y0, z0, n_soma_step)
                soma_secs[sec] = (f_pts, z0)
                soma_segment_dict.update(new_objects)
            else:
                if sec.parentsec is not None:
                    parent_sec_name.append(sec.parentsec.hname())
                else:
                    parent_sec_name.append(no_parent_count)
                    no_parent_count += 1
                branches.append(sec)
    
    return branches, parent_sec_name

@cython.wraparound(False)
@cython.boundscheck(False)
cdef tuple _process_regular_source(object source, double dx, dict soma_secs, dict soma_segment_dict, int n_soma_step):
    """Process regular source (list of sections)."""
    h.define_shape()
    cdef list branches = []
    cdef list parent_sec_name = []
    
    for sec in source:
        # TODO: make this more general (support for 3D contour outline)
        if sec.hoc_internal_name() in neuron._sec_db:
            is_stack, x, y, z, x0, y0, z0, pts3d = neuron._sec_db[sec.hoc_internal_name()]
            if not is_stack:
                shift = []
                for i, (orig_x, orig_y, orig_z) in zip([0, sec.n3d()-1], pts3d):
                    shift.append((sec.x3d(i) - orig_x,
                                  sec.y3d(i) - orig_y,
                                  sec.z3d(i) - orig_z))
                if sum([abs(s0 - s1) for s0,s1 in zip(*shift)]) > dx/10.0:
                    raise RxDException("soma rotation unsupported for voxelized somas") 
                sx, sy, sz = shift[0]
                x = (x + sx).to_python()
                y = (y + sy).to_python()
                z = (z + sz).to_python()
                new_objects, f_pts = soma_objects(x, y, z, sec, x0 + sx, y0 + sy, z0 + sz, n_soma_step)
                soma_secs[sec] = (f_pts, z0 + sz)
                soma_segment_dict.update(new_objects)
            else:
                import warnings
                warnings.warn('soma stack ignored; using centroid instead')
                branches.append(sec)
                parent_sec_name.append(None)
        else:
            branches.append(sec)
            if sec.trueparentseg():
                parent_sec_name.append(sec.trueparentseg().sec)
            else:
                parent_sec_name.append(None)
    
    return branches, parent_sec_name

@cython.wraparound(False)
@cython.boundscheck(False)
cdef tuple _extract_branch_coordinates(object branch, int k, bint source_is_import3d, object relevant_pts, dict cone_segment_dict):
    """Extract coordinates and diameters from a branch."""
    if source_is_import3d:
        x, y, z = [numpy.array(branch.raw.getrow(i).to_python()) for i in range(3)]
        d = branch.d.to_python()
    else:
        #################### modified from geometry.py ##################
        # make sure cones split across electrical segments
        if relevant_pts:
            rng = relevant_pts[k]
        else:
            rng = range(branch.n3d())

        arc3d = ([branch.arc3d(i) for i in rng])
        diam3d = ([branch.diam3d(i) for i in rng])
        x3d = ([branch.x3d(i) for i in rng])
        y3d = ([branch.y3d(i) for i in rng])
        z3d = ([branch.z3d(i) for i in rng])

        x = numpy.array([])
        y = numpy.array([])
        z = numpy.array([])
        d = numpy.array([])

        dx = branch.L / branch.nseg
        for iseg, seg in enumerate(branch):
            # get a list of all pts in the segment, including end points
            lo = iseg * dx
            hi = (iseg + 1) * dx

            pts = [lo] + [arc for arc in arc3d if lo < arc < hi] + [hi]

            diams = numpy.interp(pts, arc3d, diam3d)
            xcoords = numpy.interp(pts, arc3d, x3d)
            ycoords = numpy.interp(pts, arc3d, y3d)
            zcoords = numpy.interp(pts, arc3d, z3d)

            end = len(pts)-1
            if iseg == branch.nseg-1:
                end = len(pts)

            x = numpy.append(x, xcoords[0:end])
            y = numpy.append(y, ycoords[0:end])
            z = numpy.append(z, zcoords[0:end])
            d = numpy.append(d, diams[0:end])

            for i in range(len(pts)-1):
                #find the cone and assign it to that segment
                conecoords = (xcoords[i],ycoords[i],zcoords[i],xcoords[i+1],ycoords[i+1],zcoords[i+1])
                cone_segment_dict[conecoords] = seg
        ##########################################################
    
    return x, y, z, d

@cython.wraparound(False)
@cython.boundscheck(False)
cdef void _connect_to_soma(object x, object y, object z, object d, object psec, dict soma_secs, object branch, dict potential_soma_cones):
    """Connect branch to soma if applicable."""
    if psec in soma_secs:
        f_pts, somaz = soma_secs[psec]
        pt = (x[1], y[1], z[1])
        cp = closest_pt(pt, f_pts, somaz)
        # NEURON includes the wire point at the center; we want to connect
        # to the closest place on the soma's axis instead with full diameter
        # x, y, z, d = [cp[0]] + [X for X in x[1 :]], [cp[1]] + [Y for Y in y[1:]], [somaz] + [Z for Z in z[1:]], [d[1]] + [D for D in d[1 :]]
        x[0], y[0] = cp
        z[0] = somaz 
        d[0] = d[1]
        if branch not in potential_soma_cones:
            potential_soma_cones[branch] = []

cdef void _create_cone_objects(object x, object y, object z, object d, dict diam_corrections, list all_cones, dict pts_cones_db, dict diam_db, object branch, dict potential_soma_cones):
    from .graphicsPrimitives import Cone, Cylinder
    """Create cone or cylinder objects from coordinates."""
    cdef double x0, y0, z0, x1, y1, z1, d0, d1
    cdef double axisx, axisy, axisz, deltad, axislength
    cdef Py_ssize_t i
    
    for i in range(len(x) - 1):
        d0, d1 = d[i : i + 2]
        if (x[i] != x[i + 1] or y[i] != y[i + 1] or z[i] != z[i + 1]):
            # short section check
            #if linalg.norm((x[i + 1] - x[i], y[i + 1] - y[i], z[i + 1] - z[i])) < (d1 + d0) * 0.5:
            #    short_segs += 1
            axisx, axisy, axisz, deltad = x[i + 1] - x[i], y[i + 1] - y[i], z[i + 1] - z[i], d1 - d0
            axislength = (axisx ** 2 + axisy ** 2 + axisz ** 2) ** 0.5
            axisx /= axislength; axisy /= axislength; axisz /= axislength; deltad /= axislength
            x0, y0, z0 = x[i], y[i], z[i]
            x1, y1, z1 = x[i + 1], y[i + 1], z[i + 1]
            if (x0, y0, z0) in diam_corrections:
                d0 = diam_corrections[(x0, y0, z0)]
            if (x1, y1, z1) in diam_corrections:
                d1 = diam_corrections[(x1, y1, z1)]

            if d0 != d1:
                all_cones.append(Cone(x0, y0, z0, d0 * 0.5, x1, y1, z1, d1 * 0.5))
            else:
                all_cones.append(Cylinder(x0, y0, z0, x1, y1, z1, d1 * 0.5))
            with cython.wraparound(True):
                register(pts_cones_db, (x0, y0, z0), all_cones[-1])
                register(pts_cones_db, (x1, y1, z1), all_cones[-1])
                # if the cone is added to connect the dendrite to the soma
                # and it does not have corresponding segment
                # keep track of them here and assign them to a soma
                # segment later.
                if branch in potential_soma_cones:
                    potential_soma_cones[branch].append(all_cones[-1])
            register(diam_db, (x0, y0, z0), d0)
            register(diam_db, (x1, y1, z1), d1)

@cython.wraparound(False)
@cython.boundscheck(False)
cdef dict _check_diameter_corrections(dict diam_db, bint nouniform):
    """Check if diameter corrections are needed."""
    cdef dict diam_corrections = {}
    if not nouniform:
        # at join, should always be the size of the biggest branch
        # this is different behavior than NEURON, which continues the size of the
        # first point away from the join to the join
        for pt in diam_db:
            vals = diam_db[pt]
            if max(vals) != min(vals):
                diam_corrections[pt] = max(vals)
    return diam_corrections

@cython.wraparound(False)
@cython.boundscheck(False)
cdef void _process_left_join(object neighbor_left, object cone, double x0, double y0, double z0, double r0, double x1, double y1, double z1, double r1, double x2, double y2, double z2, double r2, 
                             object axis, object pt0, object pt1, object naxis, list objects, list joingroup, dict obj_pts_dict, list clips, double dx):
    """Process join on the left side."""
    cdef bint sharp_turn
    cdef numpy.ndarray[numpy.float_t, ndim=1] plane_normal, radial_vec, nradial_vec
    cdef list corner_pts
    cdef int my_corner_count, corner_count
    cdef object sp, c0, c1, neighbor_copy

    from .graphicsPrimitives import Sphere, Intersection, Plane, Union, Cylinder, Cone

    # no need to clip if the cones are perfectly aligned
    if all(axis == naxis):
        #parallel
        sp = Sphere(x1, y1, z1, r1)
        c0, c1 = get_infinite_cones(x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2, r2)

        sp.set_clip([c0, c1, Plane(x0, y0, z0, -naxis[0], -naxis[1], -naxis[2]), Plane(x2, y2, z2, axis[0], axis[1], axis[2])])
        objects.append(sp)
        with cython.wraparound(True):
            joingroup.append(objects[-1])
    elif r0 == r1 == r2:
        # simplest join: two non-parallel cylinders (no need for all that nastiness below)
        sp = Sphere(x1, y1, z1, r1)
        c0, c1 = get_infinite_cones(x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2, r2)
        sp.set_clip([Intersection([c0, c1, Plane(x0, y0, z0, -naxis[0], -naxis[1], -naxis[2]), Plane(x2, y2, z2, axis[0], axis[1], axis[2])])])
        objects.append(sp)
        with cython.wraparound(True):
            joingroup.append(objects[-1])
    else:
        # is the turn sharp or not
        # CTNG:joinangle
        sharp_turn = numpy.dot(axis, naxis) < 0
        # locate key vectors
        plane_normal = numpy.cross(axis, naxis)
        radial_vec = numpy.cross(plane_normal, axis)
        nradial_vec = numpy.cross(plane_normal, naxis)
        # normalize all of these
        radial_vec /= linalg.norm(radial_vec)
        nradial_vec /= linalg.norm(nradial_vec)

        # count the corners that are inside the other cone (for both ways)
        # CTNG:outsidecorners
        corner_pts = [pt1 + r1 * radial_vec, pt1 - r1 * radial_vec, pt1 + r1 * nradial_vec, pt1 - r1 * nradial_vec]
        my_corner_count = count_outside(neighbor_left, [corner_pts[0], corner_pts[1]], 0)
        corner_count = my_corner_count + count_outside(cone, [corner_pts[2], corner_pts[3]], 0)

        # if corner_count == 0, then probably all nan's from size 0 meeting size 0; ignore
        # if is 1, probably parallel; no joins
        if corner_count == 2:
            # CTNG:2outside
            # add clipped sphere; same rule if sharp or mild turn
            objects += join_outside(x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2, r2, dx)
            with cython.wraparound(True):
                joingroup.append(objects[-1])
                obj_pts_dict[objects[-1]] = corner_pts
        elif corner_count == 3:
            sp = Sphere(x1, y1, z1, r1)
            if sharp_turn:
                # CTNG:3outobtuse
                if my_corner_count == 1:
                    sp.set_clip([Plane(x1, y1, z1, -naxis[0], -naxis[1], -naxis[2])])
                else:
                    sp.set_clip([Plane(x1, y1, z1, axis[0], axis[1], axis[2])])
                objects.append(sp)
                with cython.wraparound(True):
                    joingroup.append(objects[-1])
                    obj_pts_dict[objects[-1]] = corner_pts
            else:
                # CTNG:3outacute
                objects += join_outside(x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2, r2, dx)
                with cython.wraparound(True):
                    joingroup.append(objects[-1])
                    obj_pts_dict[objects[-1]] = corner_pts
                    if my_corner_count == 1:
                        objects.append(sp)
                        objects[-1].set_clip([Plane(x0, y0, z0, -axis[0], -axis[1], -axis[2]), Plane(x2, y2, z2, naxis[0], naxis[1], naxis[2])])
                        joingroup.append(objects[-1])
                        obj_pts_dict[objects[-1]] = corner_pts
                    else:
                        objects.append(sp)
                        objects[-1].set_clip([Plane(x0, y0, z0, -axis[0], -axis[1], -axis[2]), Plane(x2, y2, z2, naxis[0], naxis[1], naxis[2])])
                        joingroup.append(objects[-1])
                        obj_pts_dict[objects[-1]] = corner_pts
        elif corner_count == 4:
            sp = Sphere(x1, y1, z1, r1)
            if sharp_turn:
                # CTNG:4outobtuse
                # join with the portions of a sphere that are outside at least one of the planes
                sp.set_clip([Union([
                    Plane(x1, y1, z1, axis[0], axis[1], axis[2]),
                    Plane(x1, y1, z1, -naxis[0], -naxis[1], -naxis[2])])])
                objects.append(sp)
                with cython.wraparound(True):
                    joingroup.append(objects[-1])
                    obj_pts_dict[objects[-1]] = corner_pts
            else:
                # CTNG:4outacute (+ 1 more)
                # join with the portions of a sphere that are outside both planes
                objects += join_outside(x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2, r2, dx)
                with cython.wraparound(True):
                    joingroup.append(objects[-1])
                    obj_pts_dict[objects[-1]] = corner_pts
                # AND clip the cone to not extend pass the union of the neighbor's plane and the neighbor
                if r0 == r1:
                    neighbor_copy = Cylinder(x0, y0, z0, x1, y1, z1, r0)
                else:
                    neighbor_copy = Cone(x0, y0, z0, r0, x1, y1, z1, r1)
                clips.append(Union([
                    Plane(x1, y1, z1, -naxis[0], -naxis[1], -naxis[2]),
                    Plane(x0, y0, z0, axis[0], axis[1], axis[2])]))

@cython.wraparound(False)
@cython.boundscheck(False)
cdef void _process_right_join(object neighbor_right, object cone, double x2, double y2, double z2, double r2, double x3, double y3, double z3, double r3, object axis, object pt2, object pt3, object naxis, list clips):
    from .graphicsPrimitives import Cylinder, Cone, Plane, Union
    """Process join on the right side."""
    cdef bint sharp_turn
    cdef numpy.ndarray[numpy.float_t, ndim=1] plane_normal, radial_vec, nradial_vec
    cdef double radial_vec_norm
    cdef int my_corner_count, corner_count
    cdef object neighbor_copy
    
    # no need to clip if the cones are perfectly aligned
    if any(axis != naxis):
        # locate key vectors
        plane_normal = numpy.cross(axis, naxis)
        radial_vec = numpy.cross(plane_normal, axis)
        radial_vec_norm = linalg.norm(radial_vec)

        # we check again because sometimes there are roundoff errors that this catches
        if radial_vec_norm:
            # is the turn sharp or not
            sharp_turn = numpy.dot(axis, naxis) < 0

            nradial_vec = numpy.cross(plane_normal, naxis)
            # normalize all of these
            radial_vec /= radial_vec_norm
            nradial_vec /= linalg.norm(nradial_vec)
            # count the corners that are inside the other cone (for both ways)
            my_corner_count = count_outside(neighbor_right, [pt2 + r2 * radial_vec, pt2 - r2 * radial_vec], 0)
            corner_count = my_corner_count + count_outside(cone, [pt2 + r2 * nradial_vec, pt2 - r2 * nradial_vec], 0)
            if corner_count == 2:
                # no clipping; already joined
                pass
            elif corner_count == 3:
                pass
            elif corner_count == 4:
                # CTNG:4outacute (+ 1 more)
                # already joined; just clip (only in mild turn case)
                if not sharp_turn:
                    if r2 == r3:
                        neighbor_copy = Cylinder(x2, y2, z2, x3, y3, z3, r3)
                    else:
                        neighbor_copy = Cone(x2, y2, z2, r2, x3, y3, z3, r3)
                    clips.append(Union([
                        Plane(x2, y2, z2, naxis[0], naxis[1], naxis[2]),
                        neighbor_copy]))

@cython.wraparound(False)
@cython.boundscheck(False)
cdef void _process_join_clipping(list all_cones, dict pts_cones_db, list objects, list join_groups, dict obj_pts_dict, double dx):
    from .graphicsPrimitives import Intersection, Union, Cylinder, Cone, Plane
    """Process join creation and clipping for all cones."""
    cdef dict cone_clip_db = {cone: [] for cone in all_cones}
    cdef bint sharp_turn
    cdef list join_items_needing_clipped = []
    cdef double x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2, r2, x3, y3, z3, r3
    cdef numpy.ndarray[numpy.float_t, ndim=1] pt0, pt1, pt2, pt3, axis, naxis
    cdef list left_neighbors, right_neighbors, clips, joingroup
    cdef object neighbor_left, neighbor_right, int_clip
    
    for cone in all_cones:
        joingroup = []
        x1, y1, z1, r1 = cone._x0, cone._y0, cone._z0, cone._r0
        x2, y2, z2, r2 = cone._x1, cone._y1, cone._z1, cone._r1
        pt1 = numpy.array([x1, y1, z1])
        pt2 = numpy.array([x2, y2, z2])
        axis = (pt2 - pt1) / linalg.norm(pt2 - pt1)
        left_neighbors = list(pts_cones_db[(x1, y1, z1)])

        right_neighbors = list(pts_cones_db[(x2, y2, z2)])
        left_neighbors.remove(cone)
        right_neighbors.remove(cone)
        if not left_neighbors:
            left_neighbors = [None]
        else:
            joingroup.append(cone)
            for neighbor in left_neighbors:
                joingroup.append(neighbor)
        if not right_neighbors: right_neighbors = [None]
        
        for neighbor_left, neighbor_right in itertools.product(left_neighbors, right_neighbors):
            clips = []
            # process the join on the "left" (end 1)
            if neighbor_left is not None:
                # any joins are created on the left pass; the right pass will only do clippings
                x0, y0, z0, r0 = neighbor_left._x0, neighbor_left._y0, neighbor_left._z0, neighbor_left._r0
                if x0 == x1 and y0 == y1 and z0 == z1:
                    x0, y0, z0, r0 = neighbor_left._x1, neighbor_left._y1, neighbor_left._z1, neighbor_left._r1
                pt0 = numpy.array([x0, y0, z0])
                naxis = (pt1 - pt0) / linalg.norm(pt1 - pt0)
                
                _process_left_join(neighbor_left, cone, x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2, r2, 
                                 axis, pt0, pt1, naxis, objects, joingroup, obj_pts_dict, clips, dx)

            if neighbor_right is not None:
                # any joins are created on the left pass; the right pass will only do clippings
                x3, y3, z3, r3 = neighbor_right._x0, neighbor_right._y0, neighbor_right._z0, neighbor_right._r0
                if x2 == x3 and y2 == y3 and z2 == z3:
                    x3, y3, z3, r3 = neighbor_right._x1, neighbor_right._y1, neighbor_right._z1, neighbor_right._r1
                pt3 = numpy.array([x3, y3, z3])
                naxis = (pt3 - pt2) / linalg.norm(pt3 - pt2)
                
                _process_right_join(neighbor_right, cone, x2, y2, z2, r2, x3, y3, z3, r3, axis, pt2, pt3, naxis, clips)

            if clips:
                int_clip = Intersection(clips)
                cone_clip_db[cone].append(int_clip)

        if joingroup:
            join_groups.append(joingroup)

    for cone in all_cones:
        clip = cone_clip_db[cone]
        if clip:
            cone.set_clip([Union(clip)])

cdef void _finalize_cone_segments(dict potential_soma_cones, dict cone_segment_dict):
    """Finalize cone segment assignments."""
    for sec,cones in potential_soma_cones.items():
        for cone in cones:
            cone_segment_dict.setdefault((cone._x0, cone._y0, cone._z0, cone._x1, cone._y1, cone._z1), sec.trueparentseg())

@cython.wraparound(False)
@cython.boundscheck(False)
def constructive_neuronal_geometry(object source, int n_soma_step, double dx, bint nouniform=False, object relevant_pts=None):
    # Initialize variables
    cdef list objects = []
    cdef dict cone_segment_dict = {}
    cdef dict soma_segment_dict = {}
    cdef list join_groups = []
    cdef dict obj_pts_dict = {}         # for returning corner points of join objects
    cdef list obj_sections = []
    cdef list cone_sections = []
    cdef dict soma_secs = {}
    cdef dict potential_soma_cones = {}

    # Determine source type and process accordingly
    cdef bint source_is_import3d = False
    cdef list branches = []
    cdef list parent_sec_name = []
    
    # TODO: come up with a better way of checking type
    if hasattr(source, 'sections'):
        source_is_import3d = True
        branches, parent_sec_name = _process_import3d_source(source, n_soma_step, soma_secs, soma_segment_dict)
    else:
        branches, parent_sec_name = _process_regular_source(source, dx, soma_secs, soma_segment_dict, n_soma_step)

    #####################################################################
    #
    # add the branches
    #
    #####################################################################
    cdef dict diam_corrections = {None: None}
    cdef Py_ssize_t k
    cdef object branch, psec

    while diam_corrections:
        all_cones = []
        pts_cones_db = {}
        diam_db = {}
        
        for (k, branch), psec in zip(enumerate(branches), parent_sec_name):
            # Extract coordinates and diameters
            x, y, z, d = _extract_branch_coordinates(branch, k, source_is_import3d, relevant_pts, cone_segment_dict)
            
            # Connect to soma if applicable
            _connect_to_soma(x, y, z, d, psec, soma_secs, branch, potential_soma_cones)
            
            # Create cone objects
            _create_cone_objects(x, y, z, d, diam_corrections, all_cones, pts_cones_db, diam_db, branch, potential_soma_cones)

        # Check for diameter corrections
        diam_corrections = _check_diameter_corrections(diam_db, nouniform)

    # Process joins and clipping
    _process_join_clipping(all_cones, pts_cones_db, objects, join_groups, obj_pts_dict, dx)
    
    # Finalize cone segment assignments
    _finalize_cone_segments(potential_soma_cones, cone_segment_dict)

    #####################################################################
    #
    # add the clipped objects to the list
    #
    #####################################################################

    #objects += all_cones

    return [objects, all_cones, cone_segment_dict, join_groups, obj_pts_dict, soma_segment_dict]

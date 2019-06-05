#cython: language_level=2
from neuron import h
import numpy
import itertools
import bisect
cimport numpy
from numpy import linalg
cimport cython
from neuron.rxd.rxdException import RxDException
import neuron

cdef extern from "math.h":
    double sqrt(double)
    double fabs(double)

from graphicsPrimitives import Sphere, Cone, Cylinder, SkewCone, Plane, Union, Intersection, SphereCone

cdef tuple seg_line_intersection(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, bint clip):
    # returns None if parallel (so None if 0 or infinitely many intersections)
    # if clip is True, requires intersection on segment; else returns line-line intersection
    cdef double denom = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1)
    if denom == 0: return None
    cdef double u = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / denom
    if clip and not (0 <= u <= 1): return None
    return (x1 + u * (x2 - x1), y1 + u * (y2 - y1))

cdef tuple convert3dto2d(double x, double y, double z, double px, double py, double pz, double xx, double xy, double xz, double yx, double yy, double yz):
    x -= px; y -= py; z -= pz
    return project(x, y, z, xx, xy, xz), project(x, y, z, yx, yy, yz)

cdef tuple closest_pt(pt, list pts, z2):
    dist = float('inf')
    closest = None
    for p in pts:
        x, y = p
        d = linalg.norm(numpy.array(pt) - numpy.array((x, y, z2)))
        if d < dist:
            dist = d
            closest = p
    return tuple(closest)

cdef double project(double fromx, double fromy, double fromz, double tox, double toy, double toz):
    """scalar projection"""
    return (fromx * tox + fromy * toy + fromz * toz) / (tox ** 2 + toy ** 2 + toz ** 2) ** 0.5
    
cdef tuple extreme_pts(list pts):
    if len(pts) < 2: raise Exception('extreme points computation failed')
    cdef double max_dist, d
    cdef tuple pt1, pt2, best_p1, best_p2
    max_dist = -1
    
    for pt1, pt2 in itertools.combinations(pts, 2):
        d = linalg.norm(numpy.array(pt1) - numpy.array(pt2))
        if d > max_dist:
            best_p1 = pt1
            best_p2 = pt2
            max_dist = d
    return best_p1, best_p2 

# helper function for maintaing the points-cones database
cdef register(dict pts_cones_db, tuple pt, cone):
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
cdef list convert2dto3d(double ptx, double pty, double x1, double y1, double z1, numpy.ndarray[numpy.float_t, ndim=1] axis, numpy.ndarray[numpy.float_t, ndim=1] radial_vec):
    return [x1 + ptx * axis[0] + pty * radial_vec[0], y1 + ptx * axis[1] + pty * radial_vec[1], z1 + ptx * axis[2] + pty * radial_vec[2]]

cdef double qsolve(double a, double b, double c):
    """solve a quadratic equation"""
    cdef double discrim = b ** 2 - 4 * a * c
    assert(discrim >= 0)
    return (-b - numpy.sqrt(discrim)) / (2 * a), (-b + numpy.sqrt(discrim)) / (2 * a)

cdef tangent_sphere(cone, int whichend):
    pt0 = numpy.array([cone._x0, cone._y0, cone._z0])
    pt1 = numpy.array([cone._x1, cone._y1, cone._z1])
    cdef double rnear, rfar, shift
    if whichend == 0:
        pt = pt0
        rnear = cone._r0
        rfar = cone._r1
        shift_sign = 1
    elif whichend == 1:
        pt = pt1
        rnear, rfar = cone._r1, cone._r0
        shift_sign = -1
    else:
        raise Exception('whichend for tangent_sphere must be 0 or 1')
    shift = (rnear * rfar - rnear ** 2) / cone.axislength
    axis = (pt1 - pt0) / cone.axislength
    result = Sphere(*(list(pt + shift_sign * shift * axis) + [numpy.sqrt(shift ** 2 + rnear ** 2)]))
    return result

@cython.boundscheck(False)
@cython.wraparound(False)
cdef tuple get_infinite_cones(double x0, double y0, double z0, double r0, double x1, double y1, double z1, double r1, double x2, double y2, double z2, double r2):
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
    #print 'for join (%g, %g, %g; %g) - (%g, %g, %g; %g) - (%g, %g, %g; %g):' % (x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2, r2)
    #print '   left_corner_count = %g; corner_count = %g' % (left_corner_count, corner_count)
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
        raise Exception('unexpected corner_counts?')
    
    return result

@cython.wraparound(False)
@cython.boundscheck(False)
def constructive_neuronal_geometry(source, int n_soma_step, double dx, nouniform=False):    
    cdef list objects = []
    cdef list soma_objects = []
    cdef dict cone_segment_dict = {}
    cdef list join_groups = []
    cdef dict obj_pts_dict = {}         # for returning corner points of join objects
    cdef list obj_sections = []
    cdef list cone_sections = []
    cdef int i, j, k
    cdef double x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, x4, y4, r0, r1, r2
    cdef double delta_x, delta_y, major_length, diam1, diam2
    cdef list pts, f_pts, f_diams, branches, parent_sec_name
    cdef dict pts_sources
    cdef tuple pt
    #cdef numpy.ndarray[numpy.float_t, ndim=1] x, y, z, xs_loop, ys_loop
    cdef int no_parent_count = 0
    
    source_is_import3d = False
    num_contours = None
    # TODO: come up with a better way of checking type
    if hasattr(source, 'sections'):
        source_is_import3d = True
        cell = source
        # probably an Import3D type
        num_contours = sum(sec.iscontour_ for sec in cell.sections)        
        if num_contours > 1:
            raise Exception('more than one contour is not currently supported')
        if num_contours <= 1:
            # CTNG:soma
            branches = []
            parent_sec_name = []
            soma_sec = None
            for sec in cell.sections:
                if sec.iscontour_:
                    soma_sec = sec.hname()
                    x, y, z = [sec.raw.getrow(i).to_python() for i in xrange(3)]
                    
                    # compute the center of the contour based on uniformly spaced points around the perimeter
                    center_vec = sec.contourcenter(sec.raw.getrow(0), sec.raw.getrow(1), sec.raw.getrow(2))
                    x0, y0, z0 = [center_vec.x[i] for i in xrange(3)]
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
                        raise Exception('multiple most extreme points')
                    extreme1 = extreme1[0]
                    extreme2 = extreme2[0]
                    major_length = linalg.norm(major_p1 - major_p2)
                    delta_x, delta_y = major_p2 - major_p1
                    delta_x /= n_soma_step
                    delta_y /= n_soma_step
                    
                    f_pts = [extreme1]
                    f_diams = [0]
                    
                    # CTNG:slicesoma
                    for i in xrange(1, n_soma_step):
                        x0, y0 = major_p1[0] + i * delta_x, major_p1[1] + i * delta_y
                        # slice in dir of minor axis
                        x1, y1 = x0 + minor.x[0], y0 + minor.x[1]
                        pts = []
                        for i in xrange(len(x)):
                            pt = seg_line_intersection(xs_loop[i], ys_loop[i], xs_loop[i + 1], ys_loop[i + 1], x0, y0, x1, y1, True)
                            if pt is not None: pts.append(pt)
                        p1, p2 = extreme_pts(pts)
                        p1, p2 = numpy.array(p1), numpy.array(p2)
                        cx, cy = (p1 + p2) / 2.
                        f_pts.append((cx, cy))
                        f_diams.append(linalg.norm(p1 - p2))
                    
                    f_pts.append(extreme2)
                    f_diams.append(0)
                          
                    segment_locs = [];            # location along soma for assigning segments 
                    path_length = 0; 
                    margin = 1/(sec.nseg()*2)   # half a segment, normalized length          
                    for i in xrange(len(f_pts) - 1):
                        pt1x, pt1y = f_pts[i]
                        pt2x, pt2y = f_pts[i + 1]
                        diam1 = f_diams[i]
                        diam2 = f_diams[i + 1]
                        thickness = numpy.sqrt((pt2x-pt1x)**2 + (pt2y-pt1y)**2)
                        segment_locs.append(path_length+(thickness/2))
                        path_length += thickness

                        objects.append(SkewCone(pt1x, pt1y, z0, diam1 * 0.5, pt1x + delta_x, pt1y + delta_y, z0, diam2 * 0.5, pt2x, pt2y, z0))
                        with cython.wraparound(True):
                            soma_objects.append(objects[-1])
                    for j, s in enumerate(soma_objects):
                        loc = segment_locs[j]/path_length       # normalized along length
                        for seg in sec:
                            if numpy.abs(seg.x - loc) <= margin:
                                cone_segment_dict[s] = seg      # for now in same seg dict; change this
                                break;         
                else:
                    if sec.parentsec is not None:
                        parent_sec_name.append(sec.parentsec.hname())
                    else:
                        parent_sec_name.append(no_parent_count)
                        no_parent_count += 1
                    branches.append(sec) 

    else:
        h.define_shape()
        soma_sec = None
        branches = []
        for sec in source:
            branches.append(sec)
        # this is ignored in this case, but needs to be same length
        # so this way no extra memory except the pointer
        parent_sec_name = branches


    #####################################################################
    #
    # add the branches
    #
    #####################################################################
    cdef dict diam_corrections = {None: None}
    cdef dict clip_copying = {}

    while diam_corrections:
        all_cones = []
        pts_cones_db = {}
        diam_db = {}
        for branch, psec in zip(branches, parent_sec_name):
            #################### modified from geometry.py ##################
            # make sure cones split across electrical segments
            arc3d = ([branch.arc3d(i) for i in range(branch.n3d())])
            diam3d = ([branch.diam3d(i) for i in range(branch.n3d())])
            x3d = ([branch.x3d(i) for i in range(branch.n3d())])
            y3d = ([branch.y3d(i) for i in range(branch.n3d())])
            z3d = ([branch.z3d(i) for i in range(branch.n3d())])

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

            """if source_is_import3d:
                x, y, z = [numpy.array(branch.raw.getrow(i).to_python()) for i in range(3)]
                d = branch.d.to_python() """   

            # make sure that all the ones that connect to the soma do in fact connect
            # do this by connecting to local center axis
            # CTNG:connectdends

            if psec == soma_sec and soma_sec is not None:
                pt = (x[1], y[1], z[1])
                cp = closest_pt(pt, f_pts, somaz)
                # NEURON includes the wire point at the center; we want to connect
                # to the closest place on the soma's axis instead with full diameter
                # x, y, z, d = [cp[0]] + [X for X in x[1 :]], [cp[1]] + [Y for Y in y[1:]], [somaz] + [Z for Z in z[1:]], [d[1]] + [D for D in d[1 :]]
                x[0], y[0] = cp
                z[0] = somaz
                d[0] = d[1]
                
                '''# cap this with a sphere for smooth joins
                sphere_cap = Sphere(x[0], y[0], z[0], d[0] * 0.5)
                # make sure sphere doesn't stick out of wrong side of cylinder
                sphere_cap.set_clip([Plane(x[1], y[1], z[1], x[1] - x[0], y[1] - y[0], z[1] - z[0])])
                objects.append(sphere_cap)
                with cython.wraparound(True):
                    soma_objects.append(objects[-1])'''
            

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
                    register(diam_db, (x0, y0, z0), d0)
                    register(diam_db, (x1, y1, z1), d1)
        
        diam_corrections = {}
        if not nouniform:
            # at join, should always be the size of the biggest branch
            # this is different behavior than NEURON, which continues the size of the
            # first point away from the join to the join
            for pt in diam_db:
                vals = diam_db[pt]
                if max(vals) != min(vals):
                    diam_corrections[pt] = max(vals)
       


    cdef dict cone_clip_db = {cone: [] for cone in all_cones}
    cdef bint sharp_turn
#    cdef dict join_counts = {'2m': 0, '2s': 0, '3m': 0, '3s': 0, '4m': 0, '4s': 0, '0m': 0, '0s': 0, '1m': 0, '1s': 0}
    join_items_needing_clipped = []
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
            # if any join needs to be subject to clips, it goes here
            join_item = None
            # process the join on the "left" (end 1)
            if neighbor_left is not None:
                # any joins are created on the left pass; the right pass will only do clippings
                x0, y0, z0, r0 = neighbor_left._x0, neighbor_left._y0, neighbor_left._z0, neighbor_left._r0
                if x0 == x1 and y0 == y1 and z0 == z1:
                    x0, y0, z0, r0 = neighbor_left._x1, neighbor_left._y1, neighbor_left._z1, neighbor_left._r1
                pt0 = numpy.array([x0, y0, z0])
                naxis = (pt1 - pt0) / linalg.norm(pt1 - pt0)
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
    #                if corner_count not in (1, 2, 3, 4):
    #                    print 'corner_count: ', corner_count, [pt1 + r1 * radial_vec, pt1 - r1 * radial_vec] + [pt1 + r1 * nradial_vec, pt1 - r1 * nradial_vec]
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

#                        join_type = '%d%s' % (corner_count, 's' if sharp_turn else 'm')
#                        join_counts[join_type] += 1
        




            if neighbor_right is not None:
                # any joins are created on the left pass; the right pass will only do clippings
                x3, y3, z3, r3 = neighbor_right._x0, neighbor_right._y0, neighbor_right._z0, neighbor_right._r0
                if x2 == x3 and y2 == y3 and z2 == z3:
                    x3, y3, z3, r3 = neighbor_right._x1, neighbor_right._y1, neighbor_right._z1, neighbor_right._r1
                pt3 = numpy.array([x3, y3, z3])
                naxis = (pt3 - pt2) / linalg.norm(pt3 - pt2)
                
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
                                #print 'cc=4: (%g, %g, %g; %g) (%g, %g, %g; %g) (%g, %g, %g; %g) ' % (x1, y1, z1, r1, x2, y2, z2, r2, x3, y3, z3, r3)
                                clips.append(Union([
                                    Plane(x2, y2, z2, naxis[0], naxis[1], naxis[2]),
                                    neighbor_copy]))
                            
            
            
            if clips:
                int_clip = Intersection(clips)
                cone_clip_db[cone].append(int_clip)

        if joingroup:
            join_groups.append(joingroup)
    
    for cone in all_cones:
        clip = cone_clip_db[cone]
        if clip:
            cone.set_clip([Union(clip)])
            
        
    #####################################################################
    #
    # add the clipped objects to the list
    #
    #####################################################################

    #objects += all_cones

    return [objects, all_cones, cone_segment_dict, join_groups, obj_pts_dict, soma_objects]
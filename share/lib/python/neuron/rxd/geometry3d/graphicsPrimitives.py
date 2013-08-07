# this contains cone, spherecone, and cylinder code translated and modified from Barbier and
# Galin 2004's example code
# see /u/ramcd/spatial/experiments/one_time_tests/2012-06-28/cone.cpp
# on 7 june 2012, the original code was available online at
#   http://jgt.akpeters.com/papers/BarbierGalin04/Cone-Sphere.zip

import bisect
from numpy import sqrt, fabs
import neuron
import ctypes

#
# connect to dll via ctypes
#
nrn_dll_sym = neuron.nrn_dll_sym


#
# declare prototypes
#

# void* geometry3d_new_Cylinder(double x0, double y0, double z0, double x1, double y1, double z1, double r);
geometry3d_new_Cylinder = nrn_dll_sym('geometry3d_new_Cylinder')
geometry3d_new_Cylinder.restype = ctypes.c_void_p
geometry3d_new_Cylinder.argtypes = [ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double]

# void geometry3d_delete_Cylinder(void* ptr);
geometry3d_delete_Cylinder = nrn_dll_sym('geometry3d_delete_Cylinder')
geometry3d_delete_Cylinder.argtypes = [ctypes.c_void_p]

# double geometry3d_Cylinder_signed_distance(void* ptr, double px, double py, double pz);
geometry3d_Cylinder_signed_distance = nrn_dll_sym('geometry3d_Cylinder_signed_distance')
geometry3d_Cylinder_signed_distance.restype = ctypes.c_double
geometry3d_Cylinder_signed_distance.argtypes = [ctypes.c_void_p, ctypes.c_double, ctypes.c_double, ctypes.c_double]

# void* geometry3d_new_Cone(double x0, double y0, double z0, double r0, double x1, double y1, double z1, double r1);
geometry3d_new_Cone = nrn_dll_sym('geometry3d_new_Cone')
geometry3d_new_Cone.restype = ctypes.c_void_p
geometry3d_new_Cone.argtypes = [ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double]

# void geometry3d_delete_Cone(void* ptr);
geometry3d_delete_Cone = nrn_dll_sym('geometry3d_delete_Cone')
geometry3d_delete_Cone.argtypes = [ctypes.c_void_p]

# double geometry3d_Cone_signed_distance(void* ptr, double px, double py, double pz);
geometry3d_Cone_signed_distance = nrn_dll_sym('geometry3d_Cone_signed_distance')
geometry3d_Cone_signed_distance.restype = ctypes.c_double
geometry3d_Cone_signed_distance.argtypes = [ctypes.c_void_p, ctypes.c_double, ctypes.c_double, ctypes.c_double]

# void* geometry3d_new_Sphere(double x, double y, double z);
geometry3d_new_Sphere = nrn_dll_sym('geometry3d_new_Sphere')
geometry3d_new_Sphere.restype = ctypes.c_void_p
geometry3d_new_Sphere.argtypes = [ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double]

# void geometry3d_delete_Sphere(void* ptr);
geometry3d_delete_Sphere = nrn_dll_sym('geometry3d_delete_Sphere')
geometry3d_delete_Sphere.argtypes = [ctypes.c_void_p]

# double geometry3d_Sphere_signed_distance(void* ptr, double px, double py, double pz);
geometry3d_Sphere_signed_distance = nrn_dll_sym('geometry3d_Sphere_signed_distance')
geometry3d_Sphere_signed_distance.restype = ctypes.c_double
geometry3d_Sphere_signed_distance.argtypes = [ctypes.c_void_p, ctypes.c_double, ctypes.c_double, ctypes.c_double]

# void* geometry3d_new_Plane(double x, double y, double z, double nx, double ny, double nz);
geometry3d_new_Plane = nrn_dll_sym('geometry3d_new_Plane')
geometry3d_new_Plane.restype = ctypes.c_void_p
geometry3d_new_Plane.argtypes = [ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double]

# void geometry3d_delete_Plane(void* ptr);
geometry3d_delete_Plane = nrn_dll_sym('geometry3d_delete_Plane')
geometry3d_delete_Plane.argtypes = [ctypes.c_void_p]

# double geometry3d_Plane_signed_distance(void* ptr, double px, double py, double pz);
geometry3d_Plane_signed_distance = nrn_dll_sym('geometry3d_Plane_signed_distance')
geometry3d_Plane_signed_distance.restype = ctypes.c_double
geometry3d_Plane_signed_distance.argtypes = [ctypes.c_void_p, ctypes.c_double, ctypes.c_double, ctypes.c_double]

# this is faster than max([a, b]) 
def max2(a, b):
    return a if a > b else b

def min2(a, b):
    return a if a < b else b

class Complement:
    def __init__(self, obj):
        self.obj = obj
    def __repr__(self):
        return 'Complement(%r)' % self.obj
    def distance(self, px, py, pz):
        return -self.obj.distance(px, py, pz)
    def starting_points(self, xs, ys, zs):
        return self.obj.starting_points(xs, ys, zs)

class Union:
    def __init__(self, objects):
        self.objects = objects
    def __repr__(self):
        return 'Union(%r)' % self.objects
    def distance(self, px, py, pz):
        # CTNG:uniondist
        return min(obj.distance(px, py, pz) for obj in self.objects)
    def starting_points(self, xs, ys, zs):
        return sum([c.starting_points(xs, ys, zs) for c in self.objects], [])

class Intersection:
    #def objects
    def __init__(self, objects):
        self.objects = objects
    def __repr__(self):
        return 'Intersection(%r)' % self.objects
    def distance(self, px, py, pz):
        # CTNG: intdist
        return max(obj.distance(px, py, pz) for obj in self.objects)
    def starting_points(self, xs, ys, zs):
        return sum([c.starting_points(xs, ys, zs) for c in self.objects], [])


class Plane:
    # def double d, mul, nx, ny, nz, px, py, pz
    def __init__(self, px, py, pz, nx, ny, nz):
        """(px, py, pz) -- a point; (nx, ny, nz) -- the normal vector"""
        self.nx = nx
        self.ny = ny
        self.nz = nz
        self.px = px
        self.py = py
        self.pz = pz
        self.geo3d_plane = geometry3d_new_Plane(px, py, pz, nx, ny, nz)
    def __del__(self):
        geometry3d_delete_Plane(self.geo3d_plane)

    def __repr__(self):
        return 'Plane(%g, %g, %g, %g, %g, %g)' % (self.px, self.py, self.pz, self.nx, self.ny, self.nz)
    def distance(self, x, y, z):
        # CTNG:planedist
        return geometry3d_Plane_signed_distance(self.geo3d_plane, x, y, z)
    def starting_points(self, xs, ys, zs):
        return [(bisect.bisect_left(xs, self.px), bisect.bisect_left(ys, self.py), bisect.bisect_left(zs, self.pz))]


class BasicPrimitive:
    def overlaps_x(self, lo, hi):
        return lo <= self.xhi and hi >= self.xlo
    def overlaps_y(self, lo, hi):
        return lo <= self.yhi and hi >= self.ylo
    def overlaps_z(self, lo, hi):
        return lo <= self.zhi and hi >= self.zlo


class Sphere(BasicPrimitive):
    # def double x, y, z, r, xlo, xhi, ylo, yhi, zlo, zhi
    # def list clips
    def __init__(self, x, y, z, r):
        self.x, self.y, self.z, self.r = x, y, z, r
        self.xlo, self.xhi = x - r, x + r
        self.ylo, self.yhi = y - r, y + r
        self.zlo, self.zhi = z - r, z + r
        self.clips = []
        self.geo3d_sphere = geometry3d_new_Sphere(x, y, z, r)
    def __del__(self):
        geometry3d_delete_Sphere(self.geo3d_sphere)
        
    def __repr__(self):
        if self.clips:
            return 'Sphere(%g, %g, %g, %g; clips=%r)' % (self.x, self.y, self.z, self.r, self.clips)
        else:
            return 'Sphere(%g, %g, %g, %g)' % (self.x, self.y, self.z, self.r)
    def distance(self, x, y, z):
        # CTNG:balldist
        d = geometry3d_Sphere_signed_distance(self.geo3d_sphere, x, y, z) #
        old_d = d
        for clip in self.clips:
            d = max2(d, clip.distance(x, y, z))
        return d
    def starting_points(self, xs, ys, zs):
        #for theta in numpy.arange(0, 2 * numpy.pi, 10):
        # TODO: this only works right if the entire object is inside the domain
        return sum([c.starting_points(xs, ys, zs) for c in self.clips], [(bisect.bisect_left(xs, self.x - self.r), bisect.bisect_left(ys, self.y), bisect.bisect_left(zs, self.z))])
    def set_clip(self, clips):
        self.clips = clips


class Cylinder(BasicPrimitive):
    # def double cx, cy, cz, r, rr, axisx, axisy, axisz, x0, y0, z0, x1, y1, z1, h
    # def double length, xlo, xhi, ylo, yhi, zlo, zhi
    # def list neighbors, clips, neighbor_regions
    def __repr__(self):
        if self.clips:
            return 'Cylinder(%g, %g, %g, %g, %g, %g, %g; clips=%r)' % (self.x0, self.y0, self.z0, self.x1, self.y1, self.z1, self.r, self.clips)
        else:
            return 'Cylinder(%g, %g, %g, %g, %g, %g, %g)' % (self.x0, self.y0, self.z0, self.x1, self.y1, self.z1, self.r)

    def __init__(self, x0, y0, z0, x1, y1, z1, r):
        self.geo3d_cylinder = geometry3d_new_Cylinder(x0, y0, z0, x1, y1, z1, r)
        self.x0, self.y0, self.z0, self.x1, self.y1, self.z1 = x0, y0, z0, x1, y1, z1
        
        self.xlo = min2(x0 - r, x1 - r)
        self.xhi = max2(x0 + r, x1 + r)
        self.ylo = min2(y0 - r, y1 - r)
        self.yhi = max2(y0 + r, y1 + r)
        self.zlo = min2(z0 - r, z1 - r)
        self.zhi = max2(z0 + r, z1 + r)
        self.neighbors = []
        self.clips = []
        self.neighbor_regions = []
        axisx, axisy, axisz = (x1 - x0, y1 - y0, z1 - z0)
        self.axislength = sqrt(axisx ** 2 + axisy ** 2 + axisz ** 2)

        # redundant memory usage, but cuts down on function calls
        self._x0, self._y0, self._z0, self._r0, self._x1, self._y1, self._z1, self._r1 = x0, y0, z0, r, x1, y1, z1, r
        self.r0 = r
        self.r1 = r
    
    def __del__(self):
        geometry3d_delete_Cylinder(self.geo3d_cylinder)

    def set_clip(self, clips):
        self.clips = clips

    def set_neighbors(self, neighbors, neighbor_regions):
        self.neighbors = neighbors
        self.neighbor_regions = neighbor_regions

    def starting_points(self, xs, ys, zs):
        # TODO: this only works right if the entire object is inside the domain
        return sum([c.starting_points(xs, ys, zs) for c in self.clips],
                   [(bisect.bisect_left(xs, self.x0), bisect.bisect_left(ys, self.y0), bisect.bisect_left(zs, self.z0)),
                    (bisect.bisect_left(xs, self.x1), bisect.bisect_left(ys, self.y1), bisect.bisect_left(zs, self.z1))])


    def distance(self, px, py, pz):
        # CTNG:cyldist
        d = geometry3d_Cylinder_signed_distance(self.geo3d_cylinder, px, py,pz)
        for clip in self.clips:
            d = max2(d, clip.distance(px, py, pz))
        return d




class SphereCone(BasicPrimitive):
    # def double x0, y0, z0, r0, x1, y1, z1, r1, rra, rrb, axisx, axisy, axisz, conelength, side1, side2
    # def double length, xlo, xhi, ylo, yhi, zlo, zhi, ha, hb, hra, hrb
    # def list clips

    def __repr__(self):
        return 'SphereCone(%g, %g, %g, %g, %g, %g, %g, %g)' % (self.x0, self.y0, self.z0, self.r0, self.x1, self.y1, self.z1, self.r1)
        
    def set_clip(self, clips):
        self.clips = clips


    def __init__(self, x0, y0, z0, r0, x1, y1, z1, r1):
        if r1 > r0:
            x0, y0, z0, r0, x1, y1, z1, r1 = x1, y1, z1, r1, x0, y0, z0, r0
            
        self.x0, self.y0, self.z0, self.r0, self.x1, self.y1, self.z1, self.r1 = x0, y0, z0, r0, x1, y1, z1, r1
        
        self.rra, self.rrb = r0 * r0, r1 * r1
        self.axisx, self.axisy, self.axisz = (x1 - x0, y1 - y0, z1 - z0)
        self.length = sqrt(self.axisx ** 2 + self.axisy ** 2 + self.axisz ** 2)
        # normalize the axis
        self.axisx /= self.length
        self.axisy /= self.length
        self.axisz /= self.length
        """
        rab = r0 - r1
        s = sqrt(self.length ** 2 - rab ** 2)
        self.ha = self.r0 * rab / self.length
        self.hb = self.r1 * rab / self.length
        self.hra = self.r0 * s / self.length
        self.hrb = self.r1 * s / self.length
        self.side1 = - self.ha / self.r0
        self.side2 = self.hra / self.r0
        self.conelength = self.length * self.hra / self.r0
        """
        self.r0 = r0
        self.r1 = r1
        self.rrb = r1 * r1
        self.rra = r0 * r0
        r0b = r0 - r1
        s = sqrt(self.length * self.length - r0b * r0b)
        self.ha = self.r0 * r0b / self.length
        self.hb = self.r1 * r0b / self.length
        self.hra = self.r0 * s / self.length
        self.hrb = self.r1 * s / self.length
        self.side1 = -self.ha / self.r0
        self.side2 = self.hra / self.r0
        self.conelength = self.length * self.hra / self.r0
        
        self.xlo = min2(x0 - r0, x1 - r1)
        self.xhi = max2(x0 + r0, x1 + r1)
        self.ylo = min2(y0 - r0, y1 - r1)
        self.yhi = max2(y0 + r0, y1 + r1)
        self.zlo = min2(z0 - r0, z1 - r1)
        self.zhi = max2(z0 + r0, z1 + r1)
        self.clips = []

    def distance_unclipped(self, px, py, pz):
        return self._distance(px, py, pz)


    def starting_points(self, xs, ys, zs):
        # TODO: this only works right if the entire object is inside the domain
        return sum([c.starting_points(xs, ys, zs) for c in self.clips], [(bisect.bisect_left(xs, self.x0 - self.axisx * self.r0), bisect.bisect_left(ys, self.y0 - self.axisy * self.r0), bisect.bisect_left(zs, self.z0 - self.axisz * self.r0))])


    def distance(self, px, py, pz):
        d = self._distance(px, py, pz)
        for clip in self.clips:
            d = max2(d, clip.distance(px, py, pz))
        return d

    def _distance(self, px, py, pz):
        # def double nx, ny, nz, y, yy, xx, ry, rx, nn, x
        nx, ny, nz = px - self.x0, py - self.y0, pz - self.z0
        y = nx * self.axisx + ny * self.axisy + nz * self.axisz
        nn = nx * nx + ny * ny + nz * nz

        yy = y * y
        xx = nn - yy
        # in principle, xx >= 0, however roundoff errors may cause trouble
        if xx < 0: xx = 0
        x = sqrt(xx)
        ry = x * self.side1 + y * self.side2
        if ry < 0:
            return sqrt(nn) - self.r0
        if ry > self.conelength:
            y = y - self.length
            yy = y * y
            nn = xx + yy
            return sqrt(nn) - self.r1
        else:
            rx = x * self.side2 - y * self.side1
            return rx - self.r0


class Cone(BasicPrimitive):
    # def double x0, y0, z0, r0, x1, y1, z1, r1, rra, rrb, axisx, axisy, axisz, conelength, side1, side2
    # def double length, xlo, xhi, ylo, yhi, zlo, zhi, cx, cy, cz, h
    # def list neighbors, clips, neighbor_regions
    # def bint reversed

    def __repr__(self):
        if self.reversed:
            order = [self.x1, self.y1, self.z1, self.r1, self.x0, self.y0, self.z0, self.r0]
        else:
            order = [self.x0, self.y0, self.z0, self.r0, self.x1, self.y1, self.z1, self.r1]
        if self.clips:
            return 'Cone(%g, %g, %g, %g, %g, %g, %g, %g; clips=%r)' % tuple(order + [self.clips])
        else:
            return 'Cone(%g, %g, %g, %g, %g, %g, %g, %g)' % tuple(order)
    
    def __del__(self):
        geometry3d_delete_Cone(self.geo3d_cone)


    def __init__(self, x0, y0, z0, r0, x1, y1, z1, r1):
        self._x0, self._y0, self._z0, self._r0, self._x1, self._y1, self._z1, self._r1 = x0, y0, z0, r0, x1, y1, z1, r1
        if r1 > r0:
            x0, y0, z0, r0, x1, y1, z1, r1 = x1, y1, z1, r1, x0, y0, z0, r0
            self.reversed = True
        else:
            self.reversed = False
            
        self.x0, self.y0, self.z0, self.r0, self.x1, self.y1, self.z1, self.r1 = x0, y0, z0, r0, x1, y1, z1, r1
        
        if r0 < 0:
            raise Exception('At least one Cone radius must be positive')
        if r1 < 0:
            axisx, axisy, axisz = (x1 - x0, y1 - y0, z1 - z0)
            length = sqrt(axisx ** 2 + axisy ** 2 + axisz ** 2)
            axisx /= length; axisy /= length; axisz /= length
            f = r1 / (r1 - r0)
            x1 -= f * axisx; y1 -= f * axisy; z1 -= f * axisz; r1 = 0
        
        self.geo3d_cone = geometry3d_new_Cone(x0, y0, z0, r0, x1, y1, z1, r1)
        axisx, axisy, axisz = (x1 - x0, y1 - y0, z1 - z0)
        self.axislength = sqrt(axisx ** 2 + axisy ** 2 + axisz ** 2)
        
        self.xlo = min2(x0 - r0, x1 - r1)
        self.xhi = max2(x0 + r0, x1 + r1)
        self.ylo = min2(y0 - r0, y1 - r1)
        self.yhi = max2(y0 + r0, y1 + r1)
        self.zlo = min2(z0 - r0, z1 - r1)
        self.zhi = max2(z0 + r0, z1 + r1)
        self.neighbors = []
        # self.cx, self.cy, self.cz = (x0 + x1) * 0.5, (y0 + y1) * 0.5, (z0 + z1) * 0.5
        # self.h = self.length * .5
        self.clips = []
        self.neighbor_regions = []
        
    def set_clip(self, clips):
        self.clips = clips

    def axis(self):
        if self.reversed:
            return -self.axisx, -self.axisy, -self.axisz
        else:
            return (self.axisx, self.axisy, self.axisz)

    def starting_points(self, xs, ys, zs):
        # TODO: this only works right if the entire object is inside the domain
        return sum([c.starting_points(xs, ys, zs) for c in self.clips],
                    [(bisect.bisect_left(xs, self.x0), bisect.bisect_left(ys, self.y0), bisect.bisect_left(zs, self.z0)),
                     (bisect.bisect_left(xs, self.x1), bisect.bisect_left(ys, self.y1), bisect.bisect_left(zs, self.z1))])

    def distance(self, px, py, pz):
        d = geometry3d_Cone_signed_distance(self.geo3d_cone, px, py, pz)
        for clip in self.clips:
            d = max2(d, clip.distance(px, py, pz))
        return d

        



class SkewCone(BasicPrimitive):
    # def double x0, y0, z0, r0, x1, y1, z1, r1, rra, rrb, axisx, axisy, axisz, conelength, side1, side2
    # def double length, xlo, xhi, ylo, yhi, zlo, zhi, sx, sy, sz, planed

    def __init__(self, x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2):
        """(x2, y2, z2) denotes point to skew (x1, y1, z1) to"""


        if r1 > r0:
            x0, y0, z0, r0, x1, y1, z1, r1, x2, y2, z2 = x2, y2, z2, r1, x2 - (x1 - x0), y2 - (y1 - y0), z2 - (z1 - z0), r0, x0, y0, z0
            
        self.x0, self.y0, self.z0, self.r0, self.x1, self.y1, self.z1, self.r1 = x0, y0, z0, r0, x1, y1, z1, r1
        self.rra, self.rrb = r0 * r0, r1 * r1
        self.axisx, self.axisy, self.axisz = (x1 - x0, y1 - y0, z1 - z0)
        
        self.length = sqrt((x1 - x0) ** 2 + (y1 - y0) ** 2 + (z1 - z0) ** 2)
        # skew data
        self.sx = (x2 - x1) / self.length
        self.sy = (y2 - y1) / self.length
        self.sz = (z2 - z1) / self.length
                
        # normalize the axis
        self.axisx /= self.length
        self.axisy /= self.length
        self.axisz /= self.length
        
        self.planed = - (self.axisx * x0 + self.axisy * y0 + self.axisz * z0)
        
        self.conelength = sqrt((r1 - r0) ** 2 + self.length ** 2)
        self.side1 = (r1 - r0) / self.conelength
        self.side2 = self.length / self.conelength
        
        self.xlo = min2(x0 - r0, x2 - r1)
        self.xhi = max2(x0 + r0, x2 + r1)
        self.ylo = min2(y0 - r0, y2 - r1)
        self.yhi = max2(y0 + r0, y2 + r1)
        self.zlo = min2(z0 - r0, z2 - r1)
        self.zhi = max2(z0 + r0, z2 + r1)

    def distance_unclipped(self, px, py, pz):
        return self.distance(px, py, pz)


    def starting_points(self, xs, ys, zs):
        # TODO: this only works right if the entire object is inside the domain
        return [(bisect.bisect_left(xs, self.x0), bisect.bisect_left(ys, self.y0), bisect.bisect_left(zs, self.z0))]
        #return sum([c.starting_points(xs, ys, zs) for c in self.clips], [(bisect.bisect_left(xs, self.x0), bisect.bisect_left(ys, self.y0), bisect.bisect_left(zs, self.z0))])
    
    def distance(self, px, py, pz):
        # CTNG:shearfrustdist
        # def double nx, ny, nz, y, yy, xx, ry, rx, dist
        
        # compute signed distance of point to plane (note: axis vector has length 1)        
        dist = px * self.axisx + py * self.axisy + pz * self.axisz + self.planed

        # deskew
        px -= dist * self.sx
        py -= dist * self.sy
        pz -= dist * self.sz        
        
        nx, ny, nz = px - self.x0, py - self.y0, pz - self.z0
        y = nx * self.axisx + ny * self.axisy + nz * self.axisz
        yy = y * y
        xx = nx * nx + ny * ny + nz * nz - yy
        # in principle, xx >= 0, however roundoff errors may cause trouble
        if xx < 0: xx = 0

        if y < 0:
            if xx < self.rra:
                return -y
            else:
                x = sqrt(xx) - self.r0
                return sqrt(x * x + yy)
        elif xx < self.rrb:
            return y - self.length
        else:
            x = sqrt(xx) - self.r0
            if y < 0:
                if x < 0:
                    return -y
                else:
                    return sqrt(x * x + yy)
            else:
                ry = x * self.side1 + y * self.side2
                if ry < 0:
                    return sqrt(x * x + yy)
                else:
                    rx = x * self.side2 - y * self.side1
                    if ry > self.conelength:
                        ry -= self.conelength
                        return sqrt(rx * rx + ry * ry)
                    else:
                        return rx
        

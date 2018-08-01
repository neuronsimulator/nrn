# this contains cone, spherecone, and cylinder code translated and modified from Barbier and
# Galin 2004's example code
# see /u/ramcd/spatial/experiments/one_time_tests/2012-06-28/cone.cpp
# on 7 june 2012, the original code was available online at
#   http://jgt.akpeters.com/papers/BarbierGalin04/Cone-Sphere.zip

import bisect
cimport cython
from neuron.rxd.rxdException import RxDException

cdef extern from "math.h":
    double sqrt(double)
    double fabs(double)


# I'm manually doing all maxes and mins here because I thought it would help
# eliminate round-off issues at the surface. it doesn't

#cdef double max(double a, double b):
#    return a if a > b else b

#cdef double min(double a, double b):
#    return a if a < b else b

cdef class Complement:
    def __init__(self, obj):
        self.obj = obj
    def __repr__(self):
        return 'Complement(%r)' % self.obj
    def distance(self, px, py, pz):
        return -self.obj.distance(px, py, pz)
    def starting_points(self, xs, ys, zs):
        return self.obj.starting_points(xs, ys, zs)
    property primitives:
        def __get__(self): return [self.obj]


cdef class Union:
    cdef list objects
    def __init__(self, list objects):
        self.objects = objects
    def __repr__(self):
        return 'Union(%r)' % self.objects
    @cython.boundscheck(False)
    @cython.wraparound(False)
    cpdef double distance(self, px, py, pz):
        # CTNG:uniondist
        dists = [obj.distance(px, py, pz) for obj in self.objects]
        cdef double d, d2
        d = dists[0]
        for d2 in dists:
            if d2 < d: d = d2
        return d
    cpdef list starting_points(self, xs, ys, zs):
        return sum([c.starting_points(xs, ys, zs) for c in self.objects], [])
    property primitives:
        def __get__(self): return sum([obj.primitives for obj in self.objects], [])


cdef class Intersection:
    cdef list objects
    def __init__(self, list objects):
        self.objects = objects
    def __repr__(self):
        return 'Intersection(%r)' % self.objects
    @cython.boundscheck(False)
    @cython.wraparound(False)        
    cpdef double distance(self, px, py, pz):
        # CTNG: intdist
        dists = [obj.distance(px, py, pz) for obj in self.objects]
        cdef double d, d2
        d = dists[0]
        for d2 in dists:
            if d2 > d: d = d2
        return d
    cpdef list starting_points(self, xs, ys, zs):
        return sum([c.starting_points(xs, ys, zs) for c in self.objects], [])
    property primitives:
        def __get__(self): return sum([obj.primitives for obj in self.objects], [])



cdef class Plane:
    cdef double d, mul, nx, ny, nz, px, py, pz
    def __init__(self, double px, double py, double pz, double nx, double ny, double nz):
        """(px, py, pz) -- a point; (nx, ny, nz) -- the normal vector"""
        self.d = - (nx * px + ny * py + nz * pz)
        self.mul = 1. / sqrt(nx ** 2 + ny ** 2 + nz ** 2)
        self.nx = nx
        self.ny = ny
        self.nz = nz
        self.px = px
        self.py = py
        self.pz = pz
    property primitives:
        def __get__(self): return []
    def __repr__(self):
        return 'Plane(%g, %g, %g, %g, %g, %g)' % (self.px, self.py, self.pz, self.nx, self.ny, self.nz)
    cpdef double distance(self, double x, double y, double z):
        return (self.nx * x + self.ny * y + self.nz * z + self.d) * self.mul
    cpdef list starting_points(self, xs, ys, zs):
        return [(bisect.bisect_left(xs, self.px), bisect.bisect_left(ys, self.py), bisect.bisect_left(zs, self.pz))]


cdef class Sphere:
    cdef double x, y, z, r, _xlo, _xhi, _ylo, _yhi, _zlo, _zhi
    cdef list clips
    property primitives:
        def __get__(self): return [self]

    def __init__(self, double x, double y, double z, double r):
        self.x, self.y, self.z, self.r = x, y, z, r
        self._xlo, self._xhi = x - r, x + r
        self._ylo, self._yhi = y - r, y + r
        self._zlo, self._zhi = z - r, z + r
        self.clips = []
    def __repr__(self):
        if self.clips:
            return 'Sphere(%g, %g, %g, %g; clips=%r)' % (self.x, self.y, self.z, self.r, self.clips)
        else:
            return 'Sphere(%g, %g, %g, %g)' % (self.x, self.y, self.z, self.r)
    property xlo:
        def __get__(self): return self._xlo
    property xhi:
        def __get__(self): return self._xhi
    property ylo:
        def __get__(self): return self._ylo
    property yhi:
        def __get__(self): return self._yhi
    property zlo:
        def __get__(self): return self._zlo
    property zhi:
        def __get__(self): return self._zhi
    cpdef double distance(self, double x, double y, double z):
        d = sqrt((x - self.x) ** 2 + (y - self.y) ** 2 + (z - self.z) ** 2) - self.r
        old_d = d
        for clip in self.clips:
            d = max(d, clip.distance(x, y, z))
        return d
    def starting_points(self, xs, ys, zs):
        #for theta in numpy.arange(0, 2 * numpy.pi, 10):
        # TODO: this only works right if the entire object is inside the domain
        return sum([c.starting_points(xs, ys, zs) for c in self.clips], [(bisect.bisect_left(xs, self.x - self.r), bisect.bisect_left(ys, self.y), bisect.bisect_left(zs, self.z))])
    cpdef bint overlaps_x(self, double lo, double hi):
        return lo <= self._xhi and hi >= self._xlo
    cpdef bint overlaps_y(self, double lo, double hi):
        return lo <= self._yhi and hi >= self._ylo
    cpdef bint overlaps_z(self, double lo, double hi):
        return lo <= self._zhi and hi >= self._zlo
    def set_clip(self, clips):
        self.clips = clips
    def get_clip(self):
        return self.clips
        


cdef class Cylinder:
    cdef double cx, cy, cz, r, rr, axisx, axisy, axisz, x0, y0, z0, x1, y1, z1, h
    cdef double length, _xlo, _xhi, _ylo, _yhi, _zlo, _zhi
    cdef list neighbors, clips, neighbor_regions
    def __repr__(self):
        if self.clips:
            return 'Cylinder(%g, %g, %g, %g, %g, %g, %g; clips=%r)' % (self.x0, self.y0, self.z0, self.x1, self.y1, self.z1, self.r, self.clips)
        else:
            return 'Cylinder(%g, %g, %g, %g, %g, %g, %g)' % (self.x0, self.y0, self.z0, self.x1, self.y1, self.z1, self.r)
    property xlo:
        def __get__(self): return self._xlo
    property xhi:
        def __get__(self): return self._xhi
    property ylo:
        def __get__(self): return self._ylo
    property yhi:
        def __get__(self): return self._yhi
    property zlo:
        def __get__(self): return self._zlo
    property zhi:
        def __get__(self): return self._zhi

    def __init__(self, double x0, double y0, double z0, double x1, double y1, double z1, double r):
        self.cx, self.cy, self.cz = (x0 + x1) * 0.5, (y0 + y1) * 0.5, (z0 + z1) * 0.5
        self.x0, self.y0, self.z0, self.x1, self.y1, self.z1 = x0, y0, z0, x1, y1, z1
        self.rr = r * r
        self.r = r
        self.axisx, self.axisy, self.axisz = (x1 - x0, y1 - y0, z1 - z0)
        self.length = sqrt(self.axisx ** 2 + self.axisy ** 2 + self.axisz ** 2)
        # normalize the axis
        self.axisx /= self.length
        self.axisy /= self.length
        self.axisz /= self.length
        
        
        self.h = self.length * 0.5
        
        self._xlo = min(x0 - r, x1 - r)
        self._xhi = max(x0 + r, x1 + r)
        self._ylo = min(y0 - r, y1 - r)
        self._yhi = max(y0 + r, y1 + r)
        self._zlo = min(z0 - r, z1 - r)
        self._zhi = max(z0 + r, z1 + r)
        self.neighbors = []
        self.clips = []
        self.neighbor_regions = []
    
    property axislength:
        def __get__(self):
            return self.length

    property _x0:
        def __get__(self):
            return self.x0

    property _y0:
        def __get__(self):
            return self.y0
    property _z0:
        def __get__(self):
            return self.z0
    property _r0:
        def __get__(self):
            return self.r
            
    property _x1:
        def __get__(self):
            return self.x1

    property _y1:
        def __get__(self):
            return self.y1
    property _z1:
        def __get__(self):
            return self.z1
    property _r1:
        def __get__(self):
            return self.r
    property primitives:
        def __get__(self): return [self]

    def get_clip(self):
        return self.clips


    def set_clip(self, clips):
        self.clips = clips
    
    def axis(self):
        return (self.axisx, self.axisy, self.axisz)

    def set_neighbors(self, neighbors, neighbor_regions):
        self.neighbors = neighbors
        self.neighbor_regions = neighbor_regions
    
    cpdef within_core(self, double px, double py, double pz):
        cdef double nx, ny, nz, y, yy, xx
        nx, ny, nz = px - self.cx, py - self.cy, pz - self.cz
        y = abs(self.axisx * nx + self.axisy * ny + self.axisz * nz)
        return y < self.h
        

    def starting_points(self, xs, ys, zs):
        # TODO: this only works right if the entire object is inside the domain
        return sum([c.starting_points(xs, ys, zs) for c in self.clips],
                    [(bisect.bisect_left(xs, self.x0), bisect.bisect_left(ys, self.y0), bisect.bisect_left(zs, self.z0)),
                     (bisect.bisect_left(xs, self.x1), bisect.bisect_left(ys, self.y1), bisect.bisect_left(zs, self.z1))])


    cpdef double distance(self, double px, double py, double pz):
        # CTNG:cyldist
        cdef double nx, ny, nz, y, yy, xx, d
        nx, ny, nz = px - self.cx, py - self.cy, pz - self.cz
        y = abs(self.axisx * nx + self.axisy * ny + self.axisz * nz)
        yy = y * y
        xx = nx * nx + ny * ny + nz * nz - yy
        if y < self.h:
            # this was wrong in my version from 2012-06-28, since did not
            # handle end-caps being closest
            d = max(y - self.h, sqrt(xx) - self.r)
        else:
            y -= self.h
            if xx < self.rr:
                d = y
            else:
                yy = y * y
                x = sqrt(xx) - self.r
                d = sqrt(yy + x * x)
                
        for clip in self.clips:
            d = max(d, clip.distance(px, py, pz))
        return d


    cpdef double _distance(self, double px, double py, double pz):
        """returns the distance to the cylinder, but not including the end plate
           if inside (and including the end plate if outside)"""
        cdef double nx, ny, nz, y, yy, xx, d
        nx, ny, nz = px - self.cx, py - self.cy, pz - self.cz
        y = abs(self.axisx * nx + self.axisy * ny + self.axisz * nz)
        yy = y * y
        xx = nx * nx + ny * ny + nz * nz - yy
        if y < self.h:
            # this is the part where we ignore the end plate if inside
            d = sqrt(xx) - self.r
        else:
            y -= self.h
            if xx < self.rr:
                d = y
            else:
                yy = y * y
                x = sqrt(xx) - self.r
                d = sqrt(yy + x * x)
                
        for clip in self.clips:
            d = max(d, clip.distance(px, py, pz))
        return d


        
    cpdef bint overlaps_x(self, double lo, double hi):
        return lo <= self._xhi and hi >= self._xlo
    cpdef bint overlaps_y(self, double lo, double hi):
        return lo <= self._yhi and hi >= self._ylo
    cpdef bint overlaps_z(self, double lo, double hi):
        return lo <= self._zhi and hi >= self._zlo



cdef class SphereCone:
    cdef double x0, y0, z0, r0, x1, y1, z1, r1, rra, rrb, axisx, axisy, axisz, conelength, side1, side2
    cdef double length, _xlo, _xhi, _ylo, _yhi, _zlo, _zhi, ha, hb, hra, hrb
    cdef list clips
    property primitives:
        def __get__(self): return [self]

    def __repr__(self):
        return 'SphereCone(%g, %g, %g, %g, %g, %g, %g, %g)' % (self.x0, self.y0, self.z0, self.r0, self.x1, self.y1, self.z1, self.r1)
        
    def set_clip(self, clips):
        self.clips = clips
    def get_clip(self):
        return self.clips


    def __init__(self, double x0, double y0, double z0, double r0, double x1, double y1, double z1, double r1):
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
        
        self._xlo = min(x0 - r0, x1 - r1)
        self._xhi = max(x0 + r0, x1 + r1)
        self._ylo = min(y0 - r0, y1 - r1)
        self._yhi = max(y0 + r0, y1 + r1)
        self._zlo = min(z0 - r0, z1 - r1)
        self._zhi = max(z0 + r0, z1 + r1)
        self.clips = []

    def starting_points(self, xs, ys, zs):
        # TODO: this only works right if the entire object is inside the domain
        return sum([c.starting_points(xs, ys, zs) for c in self.clips], [(bisect.bisect_left(xs, self.x0 - self.axisx * self.r0), bisect.bisect_left(ys, self.y0 - self.axisy * self.r0), bisect.bisect_left(zs, self.z0 - self.axisz * self.r0))])


    cpdef double distance(self, double px, double py, double pz):
        cdef double nx, ny, nz, y, yy, xx, ry, rx, nn, x, d
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
            d = sqrt(nn) - self.r0
        elif ry > self.conelength:
            y = y - self.length
            yy = y * y
            nn = xx + yy
            d = sqrt(nn) - self.r1
        else:
            rx = x * self.side2 - y * self.side1
            d = rx - self.r0
            
        for clip in self.clips:
            d = max(d, clip.distance(px, py, pz))
        return d


        
    cpdef bint overlaps_x(self, double lo, double hi):
        return lo <= self._xhi and hi >= self._xlo
    cpdef bint overlaps_y(self, double lo, double hi):
        return lo <= self._yhi and hi >= self._ylo
    cpdef bint overlaps_z(self, double lo, double hi):
        return lo <= self._zhi and hi >= self._zlo



cdef class Cone:
    cdef double x0, y0, z0, r0, x1, y1, z1, r1, rra, rrb, axisx, axisy, axisz, conelength, side1, side2
    cdef double length, _xlo, _xhi, _ylo, _yhi, _zlo, _zhi, cx, cy, cz, h
    cdef list neighbors, clips, neighbor_regions
    cdef bint reversed
    property xlo:
        def __get__(self): return self._xlo
    property xhi:
        def __get__(self): return self._xhi
    property ylo:
        def __get__(self): return self._ylo
    property yhi:
        def __get__(self): return self._yhi
    property zlo:
        def __get__(self): return self._zlo
    property zhi:
        def __get__(self): return self._zhi

    def __repr__(self):
        cdef list order
        if self.reversed:
            order = [self.x1, self.y1, self.z1, self.r1, self.x0, self.y0, self.z0, self.r0]
        else:
            order = [self.x0, self.y0, self.z0, self.r0, self.x1, self.y1, self.z1, self.r1]
        if self.clips:
            return 'Cone(%g, %g, %g, %g, %g, %g, %g, %g; clips=%r)' % tuple(order + [self.clips])
        else:
            return 'Cone(%g, %g, %g, %g, %g, %g, %g, %g)' % tuple(order)
    
    property _x0:
        def __get__(self):
            return self.x1 if self.reversed else self.x0

    property _y0:
        def __get__(self):
            return self.y1 if self.reversed else self.y0
    property _z0:
        def __get__(self):
            return self.z1 if self.reversed else self.z0
    property _r0:
        def __get__(self):
            return self.r1 if self.reversed else self.r0
            
    property _x1:
        def __get__(self):
            return self.x0 if self.reversed else self.x1

    property _y1:
        def __get__(self):
            return self.y0 if self.reversed else self.y1
    property _z1:
        def __get__(self):
            return self.z0 if self.reversed else self.z1
    property _r1:
        def __get__(self):
            return self.r0 if self.reversed else self.r1

    property axislength:
        def __get__(self):
            return self.length
    property primitives:
        def __get__(self): return [self]


    def __init__(self, double x0, double y0, double z0, double r0, double x1, double y1, double z1, double r1):
        if r1 > r0:
            x0, y0, z0, r0, x1, y1, z1, r1 = x1, y1, z1, r1, x0, y0, z0, r0
            self.reversed = True
        else:
            self.reversed = False
            
        self.x0, self.y0, self.z0, self.r0, self.x1, self.y1, self.z1, self.r1 = x0, y0, z0, r0, x1, y1, z1, r1
        
        if r0 < 0:
            raise RxDException('At least one Cone radius must be positive')
        if r1 < 0:
            axisx, axisy, axisz = (x1 - x0, y1 - y0, z1 - z0)
            length = sqrt(axisx ** 2 + axisy ** 2 + axisz ** 2)
            axisx /= length; axisy /= length; axisz /= length
            f = r1 / (r1 - r0)
            x1 -= f * axisx; y1 -= f * axisy; z1 -= f * axisz; r1 = 0
        
        self.rra, self.rrb = r0 * r0, r1 * r1
        self.axisx, self.axisy, self.axisz = (x1 - x0, y1 - y0, z1 - z0)
        self.length = sqrt(self.axisx ** 2 + self.axisy ** 2 + self.axisz ** 2)
        # normalize the axis
        self.axisx /= self.length
        self.axisy /= self.length
        self.axisz /= self.length
        
        self.conelength = sqrt((r1 - r0) ** 2 + self.length ** 2)
        self.side1 = (r1 - r0) / self.conelength
        self.side2 = self.length / self.conelength
        
        cdef double rmax = max(r0, r1)
        
        self._xlo = min(x0 - rmax, x1 - rmax)
        self._xhi = max(x0 + rmax, x1 + rmax)
        self._ylo = min(y0 - rmax, y1 - rmax)
        self._yhi = max(y0 + rmax, y1 + rmax)
        self._zlo = min(z0 - rmax, z1 - rmax)
        self._zhi = max(z0 + rmax, z1 + rmax)
        self.neighbors = []
        self.cx, self.cy, self.cz = (x0 + x1) * 0.5, (y0 + y1) * 0.5, (z0 + z1) * 0.5
        self.h = self.length * .5
        self.clips = []
        self.neighbor_regions = []
        
    def set_clip(self, clips):
        self.clips = clips
    def get_clip(self):
        return self.clips

    cpdef within_core(self, double px, double py, double pz):
        cdef double nx, ny, nz, y
        nx, ny, nz = px - self.cx, py - self.cy, pz - self.cz
        y = abs(self.axisx * nx + self.axisy * ny + self.axisz * nz)
        return y < self.h
        

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

    cpdef double distance(self, double px, double py, double pz):
        # CTNG:frustumdist
        cdef double nx, ny, nz, y, yy, xx, ry, rx, d
        nx, ny, nz = px - self.x0, py - self.y0, pz - self.z0
        y = nx * self.axisx + ny * self.axisy + nz * self.axisz
        yy = y * y
        xx = nx * nx + ny * ny + nz * nz - yy
        # in principle, xx >= 0, however roundoff errors may cause trouble
        if xx < 0: xx = 0

        if y < 0:
            # always nonnegative distance in this case
            if xx < self.rra:
                d = -y
            else:
                x = sqrt(xx) - self.r0
                d = sqrt(x * x + yy)
        elif xx < self.rrb and y > self.length:
            d = y - self.length
        else:
            x = sqrt(xx) - self.r0
            # y >= 0 always at this point (and if outside, not in the cylinder extending through the small end face)
            ry = x * self.side1 + y * self.side2
            if ry < 0:
                # if ry < 0 (and y > 0 from above), then outside the cone
                d = sqrt(x * x + yy)
            else:
                rx = x * self.side2 - y * self.side1
                if ry > self.conelength and y > self.length:
                    ry -= self.conelength
                    d = sqrt(rx * rx + ry * ry)
                else:
                    d = rx
                    if d < 0:
                        # end faces could be closer than the cone itself
                        d = max(rx, y - self.length)

        for clip in self.clips:
            d = max(d, clip.distance(px, py, pz))
        return d


    cpdef double _distance(self, double px, double py, double pz):
        """returns the distance to the frustum, but not including the end plate
           if inside (and including the end plate if outside)"""
        cdef double nx, ny, nz, y, yy, xx, ry, rx, d
        nx, ny, nz = px - self.x0, py - self.y0, pz - self.z0
        y = nx * self.axisx + ny * self.axisy + nz * self.axisz
        yy = y * y
        xx = nx * nx + ny * ny + nz * nz - yy
        # in principle, xx >= 0, however roundoff errors may cause trouble
        if xx < 0: xx = 0

        if y < 0:
            # always nonnegative distance in this case (i.e. outside)
            if xx < self.rra:
                d = -y
            else:
                x = sqrt(xx) - self.r0
                d = sqrt(x * x + yy)
        elif xx < self.rrb and y > self.length:
            # outside
            d = y - self.length
        else:
            x = sqrt(xx) - self.r0
            # y >= 0 always at this point (and if outside, not in the cylinder extending through the small end face)
            ry = x * self.side1 + y * self.side2
            if ry < 0:
                # if ry < 0 (and y > 0 from above), then outside the cone
                d = sqrt(x * x + yy)
            else:
                rx = x * self.side2 - y * self.side1
                if ry > self.conelength and y > self.length:
                    ry -= self.conelength
                    d = sqrt(rx * rx + ry * ry)
                else:
                    d = rx
                    # this is the part where we are ignoring the end faces

        for clip in self.clips:
            d = max(d, clip.distance(px, py, pz))
        return d


        
    cpdef bint overlaps_x(self, double lo, double hi):
        return lo <= self._xhi and hi >= self._xlo
    cpdef bint overlaps_y(self, double lo, double hi):
        return lo <= self._yhi and hi >= self._ylo
    cpdef bint overlaps_z(self, double lo, double hi):
        return lo <= self._zhi and hi >= self._zlo


cdef class SkewCone:
    cdef double x0, y0, z0, r0, x1, y1, z1, r1, rra, rrb, axisx, axisy, axisz, conelength, side1, side2
    cdef double length, _xlo, _xhi, _ylo, _yhi, _zlo, _zhi, sx, sy, sz, planed
    property xlo:
        def __get__(self): return self._xlo
    property xhi:
        def __get__(self): return self._xhi
    property ylo:
        def __get__(self): return self._ylo
    property yhi:
        def __get__(self): return self._yhi
    property zlo:
        def __get__(self): return self._zlo
    property zhi:
        def __get__(self): return self._zhi
    property primitives:
        def __get__(self): return [self]

    def get_clip(self):
        # TODO: change this if ever expand to allow clipping
        return []


    def __init__(self, double x0, double y0, double z0, double r0, double x1, double y1, double z1, double r1, double x2, double y2, double z2):
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
        cdef double rmax = max(r0, r1)
        
        self._xlo = min(x0 - rmax, x2 - rmax)
        self._xhi = max(x0 + rmax, x2 + rmax)
        self._ylo = min(y0 - rmax, y2 - rmax)
        self._yhi = max(y0 + rmax, y2 + rmax)
        self._zlo = min(z0 - rmax, z2 - rmax)
        self._zhi = max(z0 + rmax, z2 + rmax)



    def starting_points(self, xs, ys, zs):
        # TODO: this only works right if the entire object is inside the domain
        return [(bisect.bisect_left(xs, self.x0), bisect.bisect_left(ys, self.y0), bisect.bisect_left(zs, self.z0))]
        #return sum([c.starting_points(xs, ys, zs) for c in self.clips], [(bisect.bisect_left(xs, self.x0), bisect.bisect_left(ys, self.y0), bisect.bisect_left(zs, self.z0))])
    
    # TODO: allow clipping?
    cpdef double distance(self, double px, double py, double pz):
        # CTNG:shearfrustdist
        cdef double nx, ny, nz, y, yy, xx, ry, rx, dist
        
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
        elif xx < self.rrb and y > self.length:
            return y - self.length
        else:
            x = sqrt(xx) - self.r0
            ry = x * self.side1 + y * self.side2
            if ry < 0:
                return sqrt(x * x + yy)
            else:
                rx = x * self.side2 - y * self.side1
                if ry > self.conelength and y > self.length:
                    ry -= self.conelength
                    return sqrt(rx * rx + ry * ry)
                else:
                    if rx >= 0:
                        return rx
                    else:
                        return max(rx, y - self.length)


        
    cpdef bint overlaps_x(self, double lo, double hi):
        return lo <= self._xhi and hi >= self._xlo
    cpdef bint overlaps_y(self, double lo, double hi):
        return lo <= self._yhi and hi >= self._ylo
    cpdef bint overlaps_z(self, double lo, double hi):
        return lo <= self._zhi and hi >= self._zlo


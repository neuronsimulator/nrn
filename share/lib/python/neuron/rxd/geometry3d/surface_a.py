import ctypes
from ctypes import c_double, POINTER
import numpy
from neuron import nrn_dll_sym

"""Function to take any surface vox with information on distance of each vertex from surface 
and output approx. surface area contained in that vox"""


def surface_area(v0, v1, v2, v3, v4, v5, v6, v7, x0, x1, y0, y1, z0, z1):
    results = numpy.array([0.] * 48)
    
    find_triangles = nrn_dll_sym('find_triangles')
    find_triangles.argtypes = [c_double,c_double,c_double,c_double,c_double,c_double,c_double,c_double, #value0 - value7
        c_double, c_double, c_double, c_double, c_double, c_double, # x0, x1, y0, y1, z0, z1
        POINTER(c_double)
    ]
    
    llgramarea = nrn_dll_sym('llgramarea')
    llgramarea.argtypes = [POINTER(c_double), POINTER(c_double), POINTER(c_double)]
    llgramarea.restype = c_double
    
    results_ptr = results.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    
    count = find_triangles(v0, v1, v2, v3, v4, v5, v6, v7,
        x0, x1, y0, y1, z0, z1,
        results_ptr
    )
    
    pt0 = numpy.array([0.] * 3)
    pt1 = numpy.array([0.] * 3)
    pt2 = numpy.array([0.] * 3)
    
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
    
        area += llgramarea(pt0_ptr, pt1_ptr, pt2_ptr) / 2.
    
    return area

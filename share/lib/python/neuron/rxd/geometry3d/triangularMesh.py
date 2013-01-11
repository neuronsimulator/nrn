import neuron
import os
import ctypes
import numpy

#
# connect to dll via ctypes
#

for extension in ['', '.dll', '.so', '.dylib']:
    success = False
    try:
        nrn_dll = ctypes.cdll[os.path.join(neuron.__path__[0], 'hoc' + extension)]
        success = True
    except:
        pass
    if success: break
else:
    raise Exception('unable to connect to the NEURON library')
    
#
# declare prototypes
#

# double geometry3d_llgramarea(double* x, double* y, double* z)
geometry3d_sum_area_of_triangles = nrn_dll.geometry3d_sum_area_of_triangles
geometry3d_sum_area_of_triangles.restype = ctypes.c_double
geometry3d_sum_area_of_triangles.argtypes = [
    numpy.ctypeslib.ndpointer(dtype=numpy.float64, ndim=1, flags='C_CONTIGUOUS'),
    ctypes.c_double]


class TriangularMesh:
    """
    A triangular mesh, typically of a surface.
    """
    
    def __init__(self, data):
        """
        Parameters
        ----------
        data : :class:`numpy.ndarray` or other iterable
            The raw data, listed in the form `(x0, y0, z0, x1, y1, z1, x2, y2, z2)` repeated.
        """
        self.data = data
    
    @property
    def x(self):
        """The x coordinates of the vertices."""
        return self.data[0 :: 3]
    
    @property
    def y(self):
        """The y coordinates of the vertices."""
        return self.data[1 :: 3]
    
    @property
    def z(self):
        """The z coordinates of the vertices."""
        return self.data[2 :: 3]
    
    @property
    def faces(self):
        """A list of the triangles, described as lists of the indices of three points."""
        return [(i, i + 1, i + 2) for i in xrange(0, len(self.data) / 3, 3)]

    @property
    def area(self):
        """The sum of the areas of all the triangles."""
        return geometry3d_sum_area_of_triangles(self.data, len(self.data))

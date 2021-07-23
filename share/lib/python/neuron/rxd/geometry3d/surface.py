from . import ctng
from . import surfaces
from . import triangularMesh
import numpy
from numpy import sqrt, fabs


def surface(source, dx=0.25, internal_membranes=False, n_soma_step=100):
    """
    Generates a triangularized mesh of the surface of a neuron.

    Parameters
    ----------
    source : :func:`list`, ``nrn.SectionList``, or ``nrn.Import3D``
        The geometry to mesh.
    dx : double, optional
        Underlying mesh used to generate the triangles.
    internal_membranes : [``True`` | ``False``], optional
        Set to True to not remove internal membranes.
    n_soma_step : integer, optional
        Number of pieces to slice a soma outline into.

    Returns
    -------
    result : :class:`TriangularMesh`
        The mesh.

    Examples
    --------

    A simple meshing of the entire NEURON morphology.

    >>> tri_mesh = geometry3d.surface(h.allsec()) #doctest: +SKIP

    Importing from Neurolucida with a coarser grid.

    >>> h.load_file('stdlib.hoc')
    1.0
    >>> h.load_file('import3d.hoc')
    1.0
    >>> cell = h.Import3d_Neurolucida3()
    >>> cell.input(filename_dot_asc)
    >>> tri_mesh = geometry3d.surface(cell, dx=0.5)

    Removal of the internal membranes is not necessary if the only
    goal is to plot the surface; here we use :mod:`mayavi.mlab`.

    >>> tri_mesh = geometry3d.surface([sec1, sec2, sec3],
    ...                               internal_membranes=True)
    >>> mlab.triangular_mesh(tri_mesh.x, tri_mesh.y, tri_mesh.z,
    ...                      tri_mesh.faces, color=(1, 0, 0))
    >>> mlab.show()

    .. note::
        The use of Import3D objects is recommended over lists of sections
        because the former preserves the soma outline information while
        the later does not. Up to one soma outline is currently supported.
    """
    objects = ctng.constructive_neuronal_geometry(source, n_soma_step, dx)

    xlo = min(obj.xlo for obj in objects)
    ylo = min(obj.ylo for obj in objects)
    zlo = min(obj.zlo for obj in objects)
    xhi = max(obj.xhi for obj in objects)
    yhi = max(obj.yhi for obj in objects)
    zhi = max(obj.zhi for obj in objects)

    # I'm implicitly taking dx = dy = dz here
    # NOTE: triangulate_surface requires consistent discretization
    xs = numpy.arange(xlo - 3 * dx, xhi + 3 * dx, dx)
    ys = numpy.arange(ylo - 3 * dx, yhi + 3 * dx, dx)
    zs = numpy.arange(zlo - 3 * dx, zhi + 3 * dx, dx)

    return triangularMesh.TriangularMesh(
        surfaces.triangulate_surface(objects, xs, ys, zs, internal_membranes)
    )

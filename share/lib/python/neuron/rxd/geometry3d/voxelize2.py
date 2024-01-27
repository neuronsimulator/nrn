from . import ctng
from . import scalarField
import numpy
from .surfaces import chunkify
from . import surfaces
from numpy.linalg import norm

_max_chunks = 10000000

# TODO: should we expand this to also compute the area between neighboring nodes?


def voxelize2(
    source,
    dx=0.25,
    xlo=None,
    xhi=None,
    ylo=None,
    yhi=None,
    zlo=None,
    zhi=None,
    n_soma_step=100,
    relevant_pts=None,
):
    """
    Generates a cartesian mesh of the volume of a neuron, together with discretized information on
    surface areas and volumes. This is more accurate than voxelize, which only checks the center
    point of a voxel, but it is slower.

    Parameters
    ----------
    source : :func:`list`, ``nrn.SectionList``, or ``nrn.Import3D``
        The geometry to mesh.
    dx : double, optional
        Mesh step size.
    xlo : double, optional
        Minimum x value. If omitted or None, uses minimum x value in the geometry.
    xhi : double, optional
        Maximum x value. If omitted or None, uses maximum x value in the geometry.
    ylo : double, optional
        Minimum y value. If omitted or None, uses minimum y value in the geometry.
    yhi : double, optional
        Maximum y value. If omitted or None, uses maximum y value in the geometry.
    zlo : double, optional
        Minimum z value. If omitted or None, uses minimum z value in the geometry.
    zhi : double, optional
        Maximum z value. If omitted or None, uses maximum z value in the geometry.
    n_soma_step : integer, optional
        Number of pieces to slice a soma outline into.

    relevant_pts : list, optional
        A list for each section in sources with a list of the 3D points in that section that should be used.

    Returns
    -------
    mesh : :class:`ScalarField`
        The mesh. Values are scalars, but may be used as True inside the
        geometry and False outside.
    surface_areas : :class:`ScalarField`
        The total surface area passing through the node.
    volumes : :class:`ScalarField`
        The volume of the neuron contained in the given node.

    Examples
    --------

    Basic usage:

    >>> mesh, surface_areas, volumes = geometry3d.voxelize2(h.allsec())

    Full example, using :mod:`pyplot`:

    >>> s1, s2, s3 = [h.Section() for i in xrange(3)]
    >>> for sec in [s2, s3]: ignore_return = sec.connect(s1)
    ...
    >>> for sec in h.allsec():
    ...     sec.diam = 1
    ...     sec.L = 5
    ...
    >>> mesh = geometry3d.voxelize2(h.allsec(), dx=.1)[0]
    >>> for i in xrange(10):
    ...     ignore_return = pyplot.subplot(2, 5, i + 1)
    ...     ignore_return = pyplot.imshow(mesh.values[:, :, i])
    ...     ignore_return = pyplot.xticks([])
    ...     ignore_return = pyplot.yticks([])
    ...
    >>> pyplot.show()

    .. plot::

        from neuron import h
        from matplotlib import pyplot
        from . import geometry3d

        s1, s2, s3 = [h.Section() for i in xrange(3)]
        for sec in [s2, s3]: ignore_return = sec.connect(s1)

        for sec in h.allsec():
            sec.diam = 1
            sec.L = 5

        mesh = geometry3d.voxelize2(h.allsec(), dx=.1)[0]
        for i in xrange(10):
            ignore_return = pyplot.subplot(2, 5, i + 1)
            ignore_return = pyplot.imshow(mesh.values[:, :, i])
            ignore_return = pyplot.xticks([])
            ignore_return = pyplot.yticks([])

        pyplot.show()



    .. note::
        The use of Import3D objects is recommended over lists of sections
        because the former preserves the soma outline information while
        the later does not. Up to one soma outline is currently supported.
    """

    objects = ctng.constructive_neuronal_geometry(
        source, n_soma_step, dx, relevant_pts=relevant_pts
    )

    if xlo is None:
        xlo = min(obj.xlo for obj in objects) - 3 * dx
    if ylo is None:
        ylo = min(obj.ylo for obj in objects) - 3 * dx
    if zlo is None:
        zlo = min(obj.zlo for obj in objects) - 3 * dx
    if xhi is None:
        xhi = max(obj.xhi for obj in objects) + 3 * dx
    if yhi is None:
        yhi = max(obj.yhi for obj in objects) + 3 * dx
    if zhi is None:
        zhi = max(obj.zhi for obj in objects) + 3 * dx

    mesh = scalarField.ScalarField(xlo, xhi, ylo, yhi, zlo, zhi, dx, dtype="B")
    grid = mesh.values

    xs, ys, zs = mesh.xs, mesh.ys, mesh.zs

    # use chunks no smaller than 10 voxels across, but aim for max_chunks chunks
    chunk_size = max(
        10, int((len(mesh.xs) * len(mesh.ys) * len(mesh.zs) / _max_chunks) ** (1 / 3.0))
    )

    chunk_objs, nx, ny, nz = chunkify(
        objects, mesh.xs, mesh.ys, mesh.zs, chunk_size, dx
    )

    # this is the crude volumetric approach shared with voxelize
    for i, x in enumerate(mesh.xs):
        chunk_objsa = chunk_objs[i // chunk_size]
        for j, y in enumerate(mesh.ys):
            chunk_objsb = chunk_objsa[j // chunk_size]
            for k, z in enumerate(mesh.zs):
                grid[i, j, k] = is_inside(x, y, z, chunk_objsb[k // chunk_size])

    surface_areas = scalarField.ScalarField(xlo, xhi, ylo, yhi, zlo, zhi, dx)
    sa_grid = surface_areas.values

    # triangulate the surface
    triangles = surfaces._triangulate_surface_given_chunks(
        objects, xs, ys, zs, False, chunk_size, chunk_objs, nx, ny, nz, True, sa_grid
    )

    # for each triangle, compute the area. Add it to the appropriate spots in sa_grid
    # TODO: move this to C? or at least cythonize it?
    for tdata in triangles.reshape(len(triangles) // 9, 9):
        v0, v1, v2 = tdata[0:3], tdata[3:6], tdata[6:9]
        centerx, centery, centerz = (v0 + v1 + v2) / 3
        i, j, k = (centerx - xlo) / dx, (centery - ylo) / dx, (centerz - zlo) / dx
        sa_grid[i, j, k] += 0.5 * norm(numpy.cross(v1 - v0, v2 - v0))

    # now ensure that any grid containing surface is included in the voxelization
    # TODO: change this when supporting multiple non-overlapping regions in one 3d Domain
    grid[sa_grid.nonzero()] = 1

    volumes = scalarField.ScalarField(xlo, xhi, ylo, yhi, zlo, zhi, dx)
    volume_values = volumes.values

    # TODO: compute the correct partial volumes for nodes on the boundary

    # start out with the full volumes for every place that contains volume
    volume_values[grid.nonzero()] = dx**3

    # correct the boundary node volumes
    # surface_nodes = sa_grid.nonzero()
    # for i, j, k in zip(*surface_nodes):
    #    volume_values[i, j, k] = surfaces.volume_inside_cell(i, j, k, chunk_objs[i // chunk_size][j // chunk_size][k // chunk_size], xs, ys, zs)

    return mesh, surface_areas, volumes, triangles.reshape(len(triangles) // 9, 9)


# inside the neuron if inside of any of its parts
def is_inside(x, y, z, active_objs):
    return 1 if any(obj(x, y, z) <= 0 for obj in active_objs) else 0

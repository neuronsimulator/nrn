import neuron
import numpy


class ScalarField:
    """
    A scalar field.
    """

    def __init__(self, xlo, xhi, ylo, yhi, zlo, zhi, dx, dtype=None):
        """
        Parameters
        ----------
        xlo : double
            Minimum x value.
        xhi : double
            Maximum x value.
        ylo : double
            Minimum y value.
        yhi : double
            Maximum y value.
        zlo : double
            Minimum z value.
        zhi : double
            Maximum z value.
        dx : double
            Spatial step size.
        dtype : string, optional
            Passed to :mod:`numpy` to define the data type.

        .. note
            The ScalarField may contain locations slightly beyond `(xhi, yhi, zhi)`
            if the extrema are not evenly divisible by `dx`, however the center
            point of the voxels will not exceed the bounds.
        """
        # I'm implicitly taking dx = dy = dz here
        xs = numpy.arange(xlo, xhi, dx)
        ys = numpy.arange(ylo, yhi, dx)
        zs = numpy.arange(zlo, zhi, dx)
        # shift everything by half a voxel
        xs = xs + dx * 0.5
        ys = ys + dx * 0.5
        zs = zs + dx * 0.5
        if xs[-1] > xhi:
            xs = xs[:-1]
        if ys[-1] > yhi:
            ys = ys[:-1]
        if zs[-1] > zhi:
            zs = zs[:-1]
        self._xs = xs
        self._ys = ys
        self._zs = zs
        self._dx = dx
        self._values = numpy.zeros(
            [len(self._xs), len(self._ys), len(self._zs)], dtype=dtype
        )

    @property
    def shape(self):
        """The shape of the scalar field.

        This is a convenience property and is equal to self._values.shape."""
        return self._values.shape

    def __getitem__(self, *args):
        """
        Indexing in to the values of the mesh. This is a syntactic shorthand for `values[indices]`.

        Examples
        --------

        To return the value from `mesh` whose position is `mesh.xs[0]`, `mesh.ys[1]`, `mesh.zs[2]`:

        >>> a = mesh.__getitem__(0, 1, 2)

        Equivalently, one can use the syntactic shorthand []:

        >>> a = mesh[0, 1, 2]

        Slices are also supported:

        >>> a = mesh[0 : 2, :, 1]
        """
        return self._values(*args)

    @property
    def dx(self):
        """Discretization size of the grid."""
        return self._dx

    @property
    def xs(self):
        """The x coordinates of the centers of the voxels."""
        return self._xs

    @property
    def ys(self):
        """The y coordinates of the centers of the voxels."""
        return self._ys

    @property
    def zs(self):
        """The z coordinates of the centers of the voxels."""
        return self._zs

    @property
    def values(self):
        """State values."""
        return self._values

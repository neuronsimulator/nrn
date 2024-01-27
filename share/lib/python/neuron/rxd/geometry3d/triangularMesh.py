import neuron
import os
import numpy
import numpy.linalg

# from mayavi import mlab

# TODO: remove circular dependency
from . import surfaces


def _register_on_neighbor_map(the_map, pt, neighbor):
    # does not assume neighbor relations are bidirectional
    if pt in the_map:
        the_map[pt].append(neighbor)
    else:
        the_map[pt] = [neighbor]


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
        return self.data[0::3]

    @property
    def y(self):
        """The y coordinates of the vertices."""
        return self.data[1::3]

    @property
    def z(self):
        """The z coordinates of the vertices."""
        return self.data[2::3]

    @property
    def faces(self):
        """A list of the triangles, described as lists of the indices of three points."""
        return [(i, i + 1, i + 2) for i in range(0, len(self.data) / 3, 3)]

    @property
    def area(self):
        """The sum of the areas of the constituent triangles."""
        return surfaces.tri_area(triangles)

    def has_unmatched_edge(self, precision=3):
        """Checks for edges that belong to only one triangle. True if they exist; else False.

        Parameters
        ----------
        precision : int, optional
            Number of digits after the decimal point to round to when comparing points.
        """

        scale_factor = 10**precision

        data = self.data
        pt_neighbor_map = {}
        for i in range(0, len(self.data), 9):
            pts = {}
            for j in range(3):
                pts[
                    tuple(
                        (scale_factor * data[i + 3 * j : i + 3 * j + 3]).round()
                        / scale_factor
                    )
                ] = 0
            pts = list(pts.keys())
            if len(pts) == 3:
                # only consider triangles of nonzero area
                for j in range(3):
                    for k in range(3):
                        if j != k:
                            _register_on_neighbor_map(pt_neighbor_map, pts[j], pts[k])
            # else:
            #    print '** discarded zero-area triangle **'
        edge_count = 0
        bad_pts = []
        # if no holes, each point should have each neighbor listed more than once
        for pt, neighbor_list in zip(
            list(pt_neighbor_map.keys()), list(pt_neighbor_map.values())
        ):
            count = {}
            for neighbor in neighbor_list:
                if neighbor not in count:
                    count[neighbor] = 1
                else:
                    count[neighbor] += 1
            for neighbor, ncount in zip(list(count.keys()), list(count.values())):
                if ncount <= 1:
                    if pt < neighbor:
                        # only print one direction of it
                        edge_count += 1
                        # mlab.plot3d([pt[0], neighbor[0]], [pt[1], neighbor[1]], [pt[2], neighbor[2]], color=(0,0,1))
                        if pt not in bad_pts:
                            bad_pts.append(pt)
                        if neighbor not in bad_pts:
                            bad_pts.append(neighbor)
                        # TODO: remove this; should never get here anyways
                        print(
                            (
                                "exposed edge: (%g, %g, %g) to (%g, %g, %g)"
                                % (pt + neighbor)
                            )
                        )

        if edge_count:
            return True
        # print 'total exposed edges: ', edge_count

        # bad_x = [pt[0] for pt in bad_pts]
        # bad_y = [pt[1] for pt in bad_pts]
        # bad_z = [pt[2] for pt in bad_pts]

        # mlab.points3d(bad_x, bad_y, bad_z, scale_factor=0.05, color=(0,0,1))

        return False

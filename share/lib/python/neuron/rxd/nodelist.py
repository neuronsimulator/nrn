from .rxdException import RxDException
from .node import Node
import types
from collections import abc


class NodeList(list):
    def __init__(self, items):
        """Constructs a NodeList from items, a python iterable containing Node objects."""
        if isinstance(items, abc.Generator) or isinstance(items, abc.Iterator):
            items = list(items)

        if items == [] or all(isinstance(item, Node) for item in items):
            list.__init__(self, items)
        else:
            raise TypeError("Items must be nodes.")

    def __call__(self, restriction):
        """returns a sub-NodeList consisting of nodes satisfying restriction"""
        return NodeList([i for i in self if i.satisfies(restriction)])

    def __getitem__(self, key):
        if isinstance(key, slice):
            return NodeList(list.__getitem__(self, key))
        else:
            return list.__getitem__(self, key)

    def __setitem__(self, index, value):
        if not isinstance(value, Node):
            raise TypeError("Only assign a node to the list")
        super().__setitem__(index, value)

    def append(self, items):
        if not isinstance(items, Node):
            raise TypeError("The append item must be a Node.")
        super().append(items)

    def extend(self, items):
        if isinstance(items, abc.Generator) or isinstance(items, abc.Iterator):
            items = list(items)

        for item in items:
            if not isinstance(item, Node):
                raise TypeError("The extended items must all be Nodes.")
        super().extend(items)

    def insert(self, position, items):
        if not isinstance(items, Node):
            raise TypeError("The item inserted must be a Node.")
        super().insert(position, items)

    @property
    def value(self):
        # TODO: change this when not everything is a concentration
        return self.concentration

    @value.setter
    def value(self, v):
        # TODO: change this when not everything is a concentration
        self.concentration = v

    @property
    def segment(self):
        return [node.segment for node in self]

    @property
    def concentration(self):
        """Returns the concentration of the Node objects in the NodeList as an iterable."""
        return [node.concentration for node in self]

    @concentration.setter
    def concentration(self, value):
        """Sets the concentration of the Node objects to either a constant or values based on an iterable."""
        if hasattr(value, "__len__"):
            if len(value) == len(self):
                for node, val in zip(self, value):
                    node.concentration = val
                return
            else:
                raise RxDException(
                    "concentration must either be a scalar or an iterable of the same length as the NodeList"
                )
        for node in self:
            node.concentration = value

    @property
    def _ref_value(self):
        if not self:
            raise RxDException("no nodes")
        if len(self) != 1:
            raise RxDException("node not unique")
        return self[0]._ref_value

    @property
    def _ref_concentration(self):
        if not self:
            raise RxDException("no nodes")
        if len(self) != 1:
            raise RxDException("node not unique")
        return self[0]._ref_concentration

    @property
    def diff(self):
        """Returns the diffusion constant of the Node objects in the NodeList as an iterable."""
        return [node.diff for node in self]

    @diff.setter
    def diff(self, value):
        """Sets the diffusion constant of the Node objects to either a constant or values based on an iterable."""
        if hasattr(value, "__len__"):
            if len(value) == len(self):
                for node, val in zip(self, value):
                    node.diff = val
                return
            else:
                raise RxDException(
                    "diff must either be a scalar or an iterable of the same length as the NodeList"
                )
        for node in self:
            node.diff = value

    def include_flux(self, *args, **kwargs):
        for node in self:
            # Unpack arguments for each individual call
            node.include_flux(*args, **kwargs)

    def value_to_grid(self):
        """Returns a regular grid with the values of the 3d nodes in the list.

        The grid is a copy only.

        Grid points not belonging to the object are assigned a value of NaN.

        Nodes that are not 3d will be ignored. If there are no 3d nodes, returns
        a 0x0x0 numpy array.

        Warning: Currently only supports nodelists over 1 region.
        """
        import numpy
        from .node import Node3D

        # identify regions involved
        regions = set()
        for node in self:
            if isinstance(node, Node3D):
                regions.add(node.region)

        # default result that falls through if no regions
        result = numpy.zeros((0, 0, 0))

        if len(regions) > 1:
            # TODO: the reason for this restriction is the need to make sure
            #       the mesh lines up
            raise RxDException("value_to_grid currently only supports 1 region")

        for r in regions:
            # TODO: if allowing multiple regions, need to change this
            result = numpy.empty((max(r._xs) + 5, max(r._ys) + 5, max(r._zs) + 5))
            result.fill(numpy.nan)

            for node in self:
                if isinstance(node, Node3D):
                    # TODO: if allowing multiple regions, adjust i, j, k as needed
                    result[node._i, node._j, node._k] = node.value

        return result

    @property
    def volume(self):
        """An iterable of the volumes of the Node objects in the NodeList.

        Read only."""
        return [node.volume for node in self]

    @property
    def surface_area(self):
        """An iterable of the surface areas of the Node objects in the NodeList.

        Read only."""
        return [node.surface_area for node in self]

    @property
    def region(self):
        """An iterable of the Region objects corresponding to the Node objects in the NodeList.

        Read only."""
        return [node.region for node in self]

    @property
    def species(self):
        """An iterable of the Species objects corresponding to the Node objects in the NodeList.

        Read only."""
        return [node.species for node in self]

    @property
    def x(self):
        """An iterable of the normalized positions of the Node objects in the NodeList.

        Read only."""
        return [node.x for node in self]

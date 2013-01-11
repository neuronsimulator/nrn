class NodeList(list):
    def __init__(self, items):
        """Constructs a NodeList from items, a python iterable containing Node objects."""
        list.__init__(self, items)
    def __call__(self, restriction):
        """returns a sub-NodeList consisting of nodes satisfying restriction"""
        return NodeList([i for i in self if i.satisfies(restriction)])
    
    @property
    def concentration(self):
        """Returns the concentration of the Node objects in the NodeList as an iterable."""
        return [node.concentration for node in self]
    @concentration.setter
    def concentration(self, value):
        """Sets the concentration of the Node objects to either a constant or values based on an iterable."""
        if hasattr(value, '__len__'):
            if len(value) == len(self):
                for node, val in zip(self, value): node.concentration = val
            else:
                raise Exception('concentration must either be a scalar or an iterable of the same length as the NodeList')
        for node in self: node.concentration = value
    
    @property
    def diff(self):
        """Returns the diffusion constant of the Node objects in the NodeList as an iterable."""
        return [node.diff for node in self]
    @diff.setter
    def diff(self, value):
        """Sets the diffusion constant of the Node objects to either a constant or values based on an iterable."""
        if hasattr(value, '__len__'):
            if len(value) == len(self):
                for node, val in zip(self, value): node.diff = val
            else:
                raise Exception('diff must either be a scalar or an iterable of the same length as the NodeList')
        for node in self: node.diff = value

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

from neuron import h


class RxDSection:
    """parent class of all Section types"""

    def name(self):
        """The name of the section, as defined by HOC."""
        return self._sec.name()

    @property
    def region(self):
        """The Region containing the RxDSection."""
        return self._region

    @property
    def nseg(self):
        """The number of segments in the NEURON section.

        In some implementations, this might be different than the number of nodes in the section.
        """
        return self._nseg

    @property
    def nrn_region(self):
        """The HOC region, if any.

        'i' if the corresponding Region maps to HOC's internal concentration

        'o' if it maps to HOC's external concentration

        None otherwise.
        """

        return self._region.nrn_region

    @property
    def species(self):
        """The Species object stores on this RxDSection."""
        return self._species()

    @property
    def section_orientation(self):
        """The HOC section orientation."""
        return self._sec.orientation()

    @property
    def L(self):
        """The length of the section in microns."""
        return self._sec.L

from neuron import h, nrn

class RxDSection(nrn.Section):
    """parent class of all rxd Section types"""
    @property
    def region(self):
        """The Region containing the RxDSection."""
        return self._region
               
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
        

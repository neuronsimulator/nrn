import weakref
import rxd
from . import species, rxdmath
import numpy
from .generalizedReaction import GeneralizedReaction

class Rate(GeneralizedReaction):
    def __init__(self, species, rate, regions=None, membrane_flux=False):
        """create a rate of change for a species on a given region or set of regions
        
        if regions is None, then does it on all regions"""
        self._species = weakref.ref(species)
        self._original_rate = rate
        self._rate, self._involved_species = rxdmath._compile(rate)
        if not hasattr(regions, '__len__'):
            regions = [regions]
        self._regions = regions
        self._trans_membrane = False
        self._update_indices()
        self._membrane_flux = membrane_flux
        if membrane_flux not in (True, False):
            raise Exception('membrane_flux must be either True or False')
        if membrane_flux and regions is None:
            # TODO: rename regions to region?
            raise Exception('if membrane_flux then must specify the (unique) membrane regions')
        rxd._register_reaction(self)
    
    def __repr__(self):
        return 'Rate(%r, %r, regions=%r, membrane_flux=%r)' % (self._species, self._original_rate, self._regions, self._membrane_flux)
    
    def _update_indices(self):
        # this is called anytime the geometry changes as well as at init
        
        self._indices_dict = {}
        
        # locate the regions containing all species (including the one that changes)
        if self._species():
            active_regions = [r for r in self._regions if self._species().indices(r)]
        else:
            active_regions = []
        for sptr in self._involved_species:
            s = sptr()
            if s:
                for r in self._regions:
                    if r in active_regions and not s.indices(r):
                        del active_regions[active_regions.index(r)]
            else:
                active_regions = []
        
        # store the indices
        for sptr in self._involved_species:
            s = sptr()
            self._indices_dict[s] = sum([s.indices(r) for r in active_regions], [])
        
        self._indices = [sum([self._species().indices(r) for r in active_regions], [])]
        self._mult = [1]
        self._update_jac_cache()

    def _do_memb_scales(self):
        # TODO: does anyone still call this?
        # TODO: update self._memb_scales (this is just a dummy value to make things run)
        self._memb_scales = 1


    
    def _get_memb_flux(self, states):
        if self._membrane_flux:
            raise Exception('membrane flux due to rxd.Rate objects not yet supported')
            # TODO: refactor the inside of _evaluate so can construct args in a separate function and just get self._rate() result
            rates = self._evaluate(states)[2]
            return self._memb_scales * rates
        else:
            return []


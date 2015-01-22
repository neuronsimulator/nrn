from .rxdException import RxDException
import weakref
from . import species, rxdmath, rxd, initializer
import numpy
from .rangevar import RangeVar
import itertools
from .generalizedReaction import GeneralizedReaction

class Rate(GeneralizedReaction):
    """Declare a contribution to the rate of change of a species or other state variable.
    
    Example:
    
        constant_production = rxd.Rate(protein, k)
        
    If this was the only contribution to protein dynamics and there was no
    diffusion, the above would be equivalent to:
    
        dprotein/dt = k
        
    If there are multiple rxd.Rate objects (or an rxd.Reaction, etc) acting on
    the same species, then their effects are summed.
    """
    def __init__(self, species, rate, regions=None, membrane_flux=False):
        """create a rate of change for a species on a given region or set of regions
        
        if regions is None, then does it on all regions"""
        self._species = weakref.ref(species)
        self._original_rate = rate
        if not hasattr(regions, '__len__'):
            regions = [regions]
        self._regions = regions
        self._membrane_flux = membrane_flux
        if membrane_flux not in (True, False):
            raise RxDException('membrane_flux must be either True or False')
        if membrane_flux and regions is None:
            # TODO: rename regions to region?
            raise RxDException('if membrane_flux then must specify the (unique) membrane regions')
        self._trans_membrane = False
        rxd._register_reaction(self)
        
        # be careful, this could keep states alive
        self._original_rate = rate

        # initialize self if the rest of rxd is already initialized
        if initializer.is_initialized():
            self._do_init()

        
    def _do_init(self):
        rate = self._original_rate
        if not isinstance(rate, RangeVar):
            self._rate, self._involved_species = rxdmath._compile(rate)
        else:
            self._involved_species = [weakref.ref(species)]
        self._update_indices()
    
    def __repr__(self):
        if len(self._regions) != 1 or self._regions[0] is not None:
            regions_short = '[' + ', '.join(r._short_repr() for r in self._regions) + ']'
            return 'Rate(%s, %s, regions=%s, membrane_flux=%r)' % (self._species()._short_repr(), self._original_rate._short_repr(), regions_short, self._membrane_flux)
        else:
            return 'Rate(%s, %s, membrane_flux=%r)' % (self._species()._short_repr(), self._original_rate._short_repr(), self._membrane_flux)
    
    def _rate_from_rangevar(self, *args):
        return self._original_rate._rangevar_vec()
    
    def _update_indices(self):
        # this is called anytime the geometry changes as well as at init
        # TODO: is the above statement true?
        
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
        if isinstance(self._original_rate, RangeVar):
            nodes = []
            for sptr in self._involved_species:
                s = sptr()
                for r in active_regions:
                    if r is None:
                        nodes += s.nodes
                    else:
                        nodes += s[r].nodes
            self._original_rate._init_ptr_vectors(nodes)
            self._mult = [1]
            self._mult_extended = self._mult
            self._indices = [self._original_rate._locs]
            self._rate = self._rate_from_rangevar
            self._get_args = lambda ignore: []
            # this indicates no contribution to the jacobian
            self._jac_rows = []
            self._jac_cols = []
        else:
            # this is called anytime the geometry changes as well as at init
            # TODO: is the above statement true?
            
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
                self._indices_dict[s] = list(itertools.chain.from_iterable([s.indices(r) for r in active_regions]))
            
            self._indices = [list(itertools.chain.from_iterable([self._species().indices(r) for r in active_regions]))]
            self._mult = [1]
            self._update_jac_cache()

    def _do_memb_scales(self):
        # TODO: does anyone still call this?
        # TODO: update self._memb_scales (this is just a dummy value to make things run)
        self._memb_scales = 1


    
    def _get_memb_flux(self, states):
        if self._membrane_flux:
            raise RxDException('membrane flux due to rxd.Rate objects not yet supported')
            # TODO: refactor the inside of _evaluate so can construct args in a separate function and just get self._rate() result
            rates = self._evaluate(states)[2]
            return self._memb_scales * rates
        else:
            return []


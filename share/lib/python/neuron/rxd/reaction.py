import weakref
from . import species, rxdmath, rxd, node, initializer
import numpy
import copy
from .generalizedReaction import GeneralizedReaction, ref_list_with_mult, get_scheme_rate1_rate2_regions_custom_dynamics_mass_action
from .rxdException import RxDException

class Reaction(GeneralizedReaction):
    def __init__(self, *args, **kwargs):
        """Specify a reaction to be added to the system.
        
        Examples:
            For 2 * H + O > H2O in a mass action reaction at rate k:
                r = rxd.Reaction(2 * H + O, H2O, k)
                
            To constrain the reaction to a specified list of regions,
            say to just the extracellular space (ext) and the cytosol (cyt),
            use the regions keyword, e.g.
                r = rxd.Reaction(2 * H + O, H2O, k, regions=[ext, cyt])
            
            For a bi-directional reaction, specify a backward reaction rate.
            e.g. if kf is the forward rate and kb is the backward rate, then:
                r = rxd.Reaction(2 * H + O, H2O, kf, kb)
            
            To use dynamics other than mass-action, add that mass_action=False
            flag and put the full formula instead of a mass-action rate for
            kf (and kb). E.g. Michaelis-Menten degradation
                r = rxd.Reaction(
                    dimer, decomposed, dimer / (k + diamer), mass_action=False
                )
        """
        
        # parse the arguments
        scheme, rate1, rate2, regions, custom_dynamics, mass_action = (
            get_scheme_rate1_rate2_regions_custom_dynamics_mass_action(args, kwargs)
        )

        # TODO: verify schemes use weakrefs
        self._scheme = scheme
        if custom_dynamics is not None and mass_action is not None:
            raise RxDException('Cannot specify both custom_dynamics and mass_action.')
        elif custom_dynamics is None and mass_action is None:
            custom_dynamics = False
        elif custom_dynamics is None and mass_action is not None:
            custom_dynamics = not mass_action
        self._custom_dynamics = custom_dynamics
        if scheme._dir == '<':
            rate_f, rate_b = 0, rate1
            if rate2 is not None:
                raise RxDException('unidirectional Reaction can have only one rate constant')
        elif scheme._dir == '<>':
            rate_f, rate_b = rate1, rate2
            if rate2 is None:
                raise RxDException('bidirectional Reaction needs two rate constants')
        elif scheme._dir == '>':
            rate_f, rate_b = rate1, 0
            if rate2 is not None:
                raise RxDException('unidirectional Reaction can have only one rate constant')
        else:
            raise RxDException('unknown reaction scheme direction: %r' % scheme._dir)
        

        self._original_rate_f = rate_f
        self._original_rate_b = rate_b
        self._membrane_flux = False
        self._dir = scheme._dir
        self._custom_dynamics = custom_dynamics

        self._trans_membrane = False
        if not hasattr(regions, '__len__'):
            regions = [regions]
        self._regions = regions
        rxd._register_reaction(self)

        # initialize self if the rest of rxd is already initialized
        if initializer.is_initialized():
            self._do_init()
            self._update_indices()


    def _do_init(self):
        self._update_rates()

    def _update_rates(self):
        lhs = self._scheme._lhs._items
        rhs = self._scheme._rhs._items
        if self._dir == '<':
            # TODO: remove this limitation (probably means doing with rate_b what done with rate_f and making sure _sources and _dests are correct
            raise RxDException('pure reverse reaction currently not supported; reformulate as a forward reaction')
        
        rate_f = copy.copy(self._original_rate_f)
        rate_b = copy.copy(self._original_rate_b)
        
        if not self._custom_dynamics:
            for k, v in zip(list(lhs.keys()), list(lhs.values())):
                if v == 1:
                    rate_f *= k
                else:
                    rate_f *= k ** v
            if self._dir == '<>':
                for k, v in zip(list(rhs.keys()), list(rhs.values())):
                    if v == 1:
                        rate_b *= k
                    else:
                        rate_b *= k ** v
        rate = rate_f - rate_b
        self._sources = ref_list_with_mult(lhs)
        self._dests = ref_list_with_mult(rhs)
        self._rate, self._involved_species = rxdmath._compile(rate)
        
        trans_membrane = any(isinstance(s(), species.SpeciesOnRegion) for s in self._involved_species)
        if trans_membrane:
            raise RxDException('Reaction does not support multi-compartment dynamics. Use MultiCompartmentReaction.')

    
    @property
    def f_rate(self):
        """Get or set the forward reaction rate"""
        return self._original_rate_f
    @property
    def b_rate(self):
        """Get or set the backward reaction rate"""
        return self._original_rate_b
    @f_rate.setter
    def f_rate(self, value):
        if self._dir not in ('<>', '>'):
            raise RxDException('no forward reaction in reaction scheme')
        self._original_rate_f = value
        self._update_rates()
    @b_rate.setter
    def b_rate(self, value):
        if self._dir not in ('<>', '<'):
            raise RxDException('no backward reaction in reaction scheme')
        self._original_rate_b = value
        self._update_rates()
        
        
    
    
    
    def __repr__(self):
        if len(self._regions) != 1 or self._regions[0] is not None:
            regions_short = '[' + ', '.join(r._short_repr() for r in self._regions) + ']'
            return 'Reaction(%s, %s, rate_b=%s, regions=%s, custom_dynamics=%r)' % (self._scheme._short_repr(), self._original_rate_f._short_repr(), self._original_rate_b._short_repr(), regions_short, self._custom_dynamics)
        else:
            return 'Reaction(%s, %s, rate_b=%s, custom_dynamics=%r)' % (self._scheme._short_repr(), self._original_rate_f._short_repr(), self._original_rate_b._short_repr(), self._custom_dynamics)
    
    
    def _do_memb_scales(self):
        # nothing to do since NEVER a membrane flux
        pass 
    


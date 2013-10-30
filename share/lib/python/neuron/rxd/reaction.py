import weakref
import species
import rxdmath
import rxd
import node
import numpy
import copy
from generalizedReaction import GeneralizedReaction, ref_list_with_mult



class Reaction(GeneralizedReaction):
    def __init__(self, scheme, rate1, rate2=None, regions=None, custom_dynamics=None, mass_action=None):
        """if not custom_dynamics, then assumes mass action: multiplies rate by appropriate powers of species;
        otherwise, assumes full equations given"""
        # TODO: verify schemes use weakrefs
        self._scheme = scheme
        if not isinstance(scheme, rxdmath._Reaction):
            raise Exception('%r not a recognized reaction scheme' % self._scheme)
        if custom_dynamics is not None and mass_action is not None:
            raise Exception('Cannot specify both custom_dynamics and mass_action.')
        elif custom_dynamics is None and mass_action is None:
            custom_dynamics = False
        elif custom_dynamics is None and mass_action is not None:
            custom_dynamics = not mass_action
        self._custom_dynamics = custom_dynamics
        if scheme._dir == '<':
            rate_f, rate_b = 0, rate1
            if rate2 is not None:
                raise Exception('unidirectional Reaction can have only one rate constant')
        elif scheme._dir == '<>':
            rate_f, rate_b = rate1, rate2
            if rate2 is None:
                raise Exception('bidirectional Reaction needs two rate constants')
        elif scheme._dir == '>':
            rate_f, rate_b = rate1, 0
            if rate2 is not None:
                raise Exception('unidirectional Reaction can have only one rate constant')
        else:
            raise Exception('unknown reaction scheme direction: %r' % scheme._dir)
        

        self._original_rate_f = rate_f
        self._original_rate_b = rate_b
        self._membrane_flux = False
        self._dir = scheme._dir
        self._custom_dynamics = custom_dynamics
        self._update_rates()


        self._trans_membrane = False
        if not hasattr(regions, '__len__'):
            regions = [regions]
        self._regions = regions
        #self._update_indices()
        rxd._register_reaction(self)

    def _update_rates(self):
        lhs = self._scheme._lhs._items
        rhs = self._scheme._rhs._items
        if self._dir == '<':
            # TODO: remove this limitation (probably means doing with rate_b what done with rate_f and making sure _sources and _dests are correct
            raise Exception('pure reverse reaction currently not supported; reformulate as a forward reaction')
        
        rate_f = copy.copy(self._original_rate_f)
        rate_b = copy.copy(self._original_rate_b)
        
        if not self._custom_dynamics:
            for k, v in zip(lhs.keys(), lhs.values()):
                if v == 1:
                    rate_f *= k
                else:
                    rate_f *= k ** v
            if self._dir == '<>':
                for k, v in zip(rhs.keys(), rhs.values()):
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
            raise Exception('Reaction does not support multi-compartment dynamics. Use MultiCompartmentReaction.')

    
    @property
    def f_rate(self):
        return self._original_rate_f
    @property
    def b_rate(self):
        return self._original_rate_b
    @f_rate.setter
    def f_rate(self, value):
        if self._dir not in ('<>', '>'):
            raise Exception('no forward reaction in reaction scheme')
        self._original_rate_f = value
        self._update_rates()
    @b_rate.setter
    def b_rate(self, value):
        if self._dir not in ('<>', '<'):
            raise Exception('no backward reaction in reaction scheme')
        self._original_rate_b = value
        self._update_rates()
        
        
    
    
    
    def __repr__(self):
        return 'Reaction(%r, %r, rate_b=%r, regions=%r, custom_dynamics=%r)' % (self._scheme, self._original_rate_f, self._original_rate_b, self._regions, self._custom_dynamics)
    
    
    def _do_memb_scales(self):
        # nothing to do since NEVER a membrane flux
        pass 
    


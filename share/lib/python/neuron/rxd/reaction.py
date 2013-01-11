import weakref
import species
import rxdmath
import rxd
import node
import numpy
from generalizedReaction import GeneralizedReaction, ref_list_with_mult



class Reaction(GeneralizedReaction):
    def __init__(self, scheme, rate_f, rate_b=None, regions=None, custom_dynamics=False):
        """if not custom_dynamics, then assumes mass action: multiplies rate by appropriate powers of species;
        otherwise, assumes full equations given"""
        # TODO: verify schemes use weakrefs
        self._scheme = scheme
        self._custom_dynamics = custom_dynamics
        self._original_rate_f = rate_f
        self._original_rate_b = rate_b
        self._membrane_flux = False
        if not isinstance(scheme, rxdmath._Reaction):
            raise Exception('%r not a recognized reaction scheme' % self._scheme)
        self._dir = scheme._dir
        lhs = scheme._lhs._items
        rhs = scheme._rhs._items
        if not custom_dynamics:
            for k, v in zip(lhs.keys(), lhs.values()):
                if v == 1:
                    rate_f *= k
                else:
                    rate_f *= k ** v
        if rate_b is not None:
            if self._dir in ('<', '>'):
                raise Exception('unidirectional Reaction can have only one rate constant')
            if not custom_dynamics:
                for k, v in zip(rhs.keys(), rhs.values()):
                    if v == 1:
                        rate_b *= k
                    else:
                        rate_b *= k ** v
            rate = rate_f - rate_b
            self._sources = ref_list_with_mult(lhs)
            self._dests = ref_list_with_mult(rhs)
        elif self._dir == '<>':
            raise Exception('bidirectional Reaction needs two rate constants')
        elif self._dir == '>':
            rate = rate_f
            self._sources = ref_list_with_mult(lhs)
            self._dests = ref_list_with_mult(rhs)
        elif self._dir == '<':
            self._sources = ref_list_with_mult(rhs)
            self._dests = ref_list_with_mult(lhs)
            rate = rate_f
        else:
            raise Exception('unrecognized direction; should never happen')
        self._rate, self._involved_species = rxdmath._compile(rate)
        trans_membrane = any(isinstance(s(), species.SpeciesOnRegion) for s in self._involved_species)
        if trans_membrane:
            raise Exception('Reaction does not support multi-compartment dynamics. Use MultiCompartmentReaction.')
        self._trans_membrane = False
        if not hasattr(regions, '__len__'):
            regions = [regions]
        self._regions = regions
        #self._update_indices()
        rxd._register_reaction(self)
    
    def __repr__(self):
        return 'Reaction(%r, %r, rate_b=%r, regions=%r, custom_dynamics=%r)' % (self._scheme, self._original_rate_f, self._original_rate_b, self._regions, self._custom_dynamics)
    
    
    def _do_memb_scales(self):
        # nothing to do since NEVER a membrane flux
        pass 
    
    def _evaluate(self, states):
        """returns: (list of lists (lol) of increase indices, lol of decr indices, list of changes)"""
        args = self._get_args(states)
        if args is None: return 
        return (self._indices, self._mult, self._rate(*args))

    


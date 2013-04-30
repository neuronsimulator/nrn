import weakref
import species
import rxdmath
import rxd
import node
import numpy
from generalizedReaction import GeneralizedReaction

def _ref_list_with_mult(obj):
    result = []
    for i, p in zip(obj.keys(), obj.values()):
        w = weakref.ref(i)
        result += [w] * p
    return result

class MultiCompartmentReaction(GeneralizedReaction):
    def __init__(self, scheme, rate_f, rate_b=None, membrane=None, custom_dynamics=False, membrane_flux=False, scale_by_area=True):
        """if not custom_dynamics, then assumes mass action: multiplies rate by appropriate powers of species;
        otherwise, assumes full equations given"""
        # TODO: verify schemes use weakrefs
        self._scheme = scheme
        self._scale_by_area = scale_by_area
        self._original_rate_f = rate_f
        self._original_rate_b = rate_b
        self._custom_dynamics = custom_dynamics
        self._trans_membrane = True
        if membrane_flux not in (True, False):
            raise Exception('membrane_flux must be either True or False')
        if membrane is None:
            raise Exception('MultiCompartmentReaction requires a membrane parameter')
        if membrane_flux:
            raise Exception('membrane_flux not supported (not tested... might work, might not, so blocked out for now...')
        self._membrane_flux = membrane_flux
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
            self._sources = _ref_list_with_mult(lhs)
            self._dests = _ref_list_with_mult(rhs)
        elif self._dir == '<>':
            raise Exception('bidirectional Reaction needs two rate constants')
        elif self._dir == '>':
            rate = rate_f
            self._sources = _ref_list_with_mult(lhs)
            self._dests = _ref_list_with_mult(rhs)
        elif self._dir == '<':
            self._sources = _ref_list_with_mult(rhs)
            self._dests = _ref_list_with_mult(lhs)
            rate = rate_f
        else:
            raise Exception('unrecognized direction; should never happen')
        self._rate, self._involved_species = rxdmath._compile(rate)
        if not all(isinstance(s(), species.SpeciesOnRegion) for s in self._involved_species):
            raise Exception('must specify region for all involved species')
        if not membrane._geometry.is_area():
            raise Exception('must specify a membrane not a volume for the boundary')
        self._regions = [membrane]
        #self._update_indices()
        rxd._register_reaction(self)
    
    def __repr__(self):
        return 'MultiCompartmentReaction(%r, %r, rate_b=%r, membrane=%r, custom_dynamics=%r, membrane_flux=%r, scale_by_area=%r)' % (self._scheme, self._original_rate_f, self._original_rate_b, self._regions[0], self._custom_dynamics, self._membrane_flux, self._scale_by_area)
    
    
    def _do_memb_scales(self):                    
        if not self._scale_by_area:
            areas = numpy.ones(len(areas))
        else:
            areas = numpy.array(sum([list(self._regions[0]._geometry.volumes1d(sec) for sec in self._regions[0].secs)], []))
        neuron_areas = []
        for sec in self._regions[0].secs:
            neuron_areas += [h.area((i + 0.5) / sec.nseg, sec=sec) for i in xrange(sec.nseg)]
        neuron_areas = numpy.array(neuron_areas)
        area_ratios = areas / neuron_areas
        # still needs to be multiplied by the valence of each molecule
        self._memb_scales = -area_ratios / (100 * molecules_per_mM_um3)
    
        

import weakref
import species
import rxdmath
import rxd
import node
import numpy
import region
from generalizedReaction import GeneralizedReaction, molecules_per_mM_um3
from neuron import h
import itertools
import copy

FARADAY = h.FARADAY

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
        if membrane_flux and region._sim_dimension != 1:
            raise Exception('membrane_flux not supported except in 1D')
        self._membrane_flux = membrane_flux
        if not isinstance(scheme, rxdmath._Reaction):
            raise Exception('%r not a recognized reaction scheme' % self._scheme)
        self._dir = scheme._dir
        self._update_rates()
        if not membrane._geometry.is_area():
            raise Exception('must specify a membrane not a volume for the boundary')
        self._regions = [membrane]
        #self._update_indices()
        rxd._register_reaction(self)


    def _update_rates(self):
        lhs = self._scheme._lhs._items
        rhs = self._scheme._rhs._items
        rate_f = copy.copy(self._original_rate_f)
        rate_b = copy.copy(self._original_rate_b)        
        if not self._custom_dynamics:
            for k, v in zip(lhs.keys(), lhs.values()):
                if v == 1:
                    rate_f *= k
                else:
                    rate_f *= k ** v
        if rate_b is not None:
            if self._dir in ('<', '>'):
                raise Exception('unidirectional Reaction can have only one rate constant')
            if not self._custom_dynamics:
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
        self._changing_species = list(set(self._sources + self._dests))
        if not all(isinstance(s(), species.SpeciesOnRegion) for s in self._involved_species):
            raise Exception('must specify region for all involved species')



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
        return 'MultiCompartmentReaction(%r, %r, rate_b=%r, membrane=%r, custom_dynamics=%r, membrane_flux=%r, scale_by_area=%r)' % (self._scheme, self._original_rate_f, self._original_rate_b, self._regions[0], self._custom_dynamics, self._membrane_flux, self._scale_by_area)
    
    
    def _do_memb_scales(self, cur_map):                    
        if not self._scale_by_area:
            areas = numpy.ones(len(areas))
        else:
            areas = numpy.array(sum([list(self._regions[0]._geometry.volumes1d(sec) for sec in self._regions[0].secs)], []))
        neuron_areas = []
        for sec in self._regions[0].secs:
            neuron_areas += [h.area((i + 0.5) / sec.nseg, sec=sec) for i in xrange(sec.nseg)]
        neuron_areas = numpy.array(neuron_areas)
        # area_ratios is usually a vector of 1s
        area_ratios = areas / neuron_areas
        # still needs to be multiplied by the valence of each molecule
        self._memb_scales = -area_ratios * FARADAY / (10000 * molecules_per_mM_um3)
        #print area_ratios
        #print self._memb_scales
        #import sys
        #sys.exit()
        
        # since self._memb_scales is only used to compute currents as seen by the rest of NEURON,
        # we only use NEURON's areas 
        #self._memb_scales = volume * molecules_per_mM_um3 / areas
        
        
        if self._membrane_flux:
            # TODO: don't assume/require always inside/outside on one side...
            #       if no nrn_region specified, then (make so that) no contribution
            #       to membrane flux
            source_regions = [s()._region()._nrn_region for s in self._sources]
            dest_regions = [d()._region()._nrn_region for d in self._dests]
            
            if 'i' in source_regions and 'o' not in source_regions and 'i' not in dest_regions:
                inside = -1 #'source'
            elif 'o' in source_regions and 'i' not in source_regions and 'o' not in dest_regions:
                inside = 1 # 'dest'
            elif 'i' in dest_regions and 'o' not in dest_regions and 'i' not in source_regions:
                inside = 1 # 'dest'
            elif 'o' in dest_regions and 'i' not in dest_regions and 'o' not in source_regions:
                inside = -1 # 'source'
            else:
                raise Exception('unable to identify which side of reaction is inside (hope to remove the need for this')
        
        # dereference the species to get the true species if it's actually a SpeciesOnRegion
        sources = [s()._species() for s in self._sources]
        dests = [d()._species() for d in self._dests]
        if self._membrane_flux:
            if any(s in dests for s in sources) or any(d in sources for d in dests):
                # TODO: remove this limitation
                raise Exception('current fluxes do not yet support same species on both sides of reaction')
        
        # TODO: make so don't need multiplicity (just do in one pass)
        # TODO: this needs changed when I switch to allowing multiple sides on the left/right (e.g. simplified Na/K exchanger)
        self._cur_charges = tuple([-inside * s.charge for s in sources if s.name is not None] + [inside * s.charge for s in dests if s.name is not None])
        self._net_charges = sum(self._cur_charges)
        
        self._cur_ptrs = []
        self._cur_mapped = []
        
        for sec in self._regions[0].secs:
            for i in xrange(sec.nseg):
                local_ptrs = []
                local_mapped = []
                for sp in itertools.chain(self._sources, self._dests):
                    spname = sp()._species().name
                    if spname is not None:
                        name = '_ref_i%s' % (spname)
                        seg = sec((i + 0.5) / sec.nseg)
                        local_ptrs.append(seg.__getattribute__(name))
                        uberlocal_map = [None, None]
                        if spname + 'i' in cur_map:
                            uberlocal_map[0] = cur_map[spname + 'i'][seg]
                        if spname + 'o' in cur_map:
                            uberlocal_map[1] = cur_map[spname + 'o'][seg]
                        local_mapped.append(uberlocal_map)
                self._cur_ptrs.append(tuple(local_ptrs))
                self._cur_mapped.append(tuple(local_mapped))

        

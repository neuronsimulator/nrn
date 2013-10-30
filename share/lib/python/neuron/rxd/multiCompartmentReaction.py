import weakref
import rxd
from . import rxdmath, node, species
import numpy
from .generalizedReaction import GeneralizedReaction, get_scheme_rate1_rate2_regions_custom_dynamics_mass_action
from neuron import h

FARADAY = h.FARADAY

def _ref_list_with_mult(obj):
    result = []
    for i, p in zip(obj.keys(), obj.values()):
        w = weakref.ref(i)
        result += [w] * p
    return result

class MultiCompartmentReaction(GeneralizedReaction):
    def __init__(self, *args, **kwargs):
        """Specify a reaction spanning multiple regions to be added to the system.
        
        Use this for, for example, pumps and channels, or interactions between
        species living in a volume (e.g. the cytosol) and species on a
        membrane (e.g. the plasma membrane).
        
        For each species/state/parameter, you must specify what region you are
        referring to, as it could be present in multiple regions. You must
        also specify a `membrane` or a `border` (these are treated as synonyms)
        that separates the regions involved in your reaction. This is necessary
        because the default behavior is to scale the reaction rate by the
        border area, as would be expected if one of the species involved is a
        pump that is binding to a species in the volume. If this is not the
        desired behavior, pass the keyword argument `scale_by_area=False`.
        
        Pass in `membrane_flux=True` if the reaction produces a current across
        the plasma membrane that should affect the membrane potential.
        
        Unlike Reaction objects, the base units for the rates are in terms of
        molecules per square micron per ms.
        
        .. seealso::
        
            :class:`neuron.rxd.Reaction`
        """
        
        # parse the arguments shared with rxd.Reaction
        scheme, rate_f, rate_b, regions, custom_dynamics, mass_action = (
            get_scheme_rate1_rate2_regions_custom_dynamics_mass_action(args, kwargs)
        )
        # additional keyword arguments
        membrane_flux = kwargs.get('membrane_flux', False)
        membrane = kwargs.get('membrane')
        border = kwargs.get('border')
        scale_by_area = kwargs.get('scale_by_area', True)

        if border is not None and membrane is not None:
            raise Exception('cannot specify both border and membrane; they are synoyms')
        if border is not None:
            membrane = border

        
        # TODO: verify schemes use weakrefs
        self._scheme = scheme
        self._scale_by_area = scale_by_area
        self._original_rate_f = rate_f
        self._original_rate_b = rate_b
        if custom_dynamics is not None and mass_action is not None:
            raise Exception('Cannot specify both custom_dynamics and mass_action.')
        elif custom_dynamics is None and mass_action is None:
            custom_dynamics = False
        elif custom_dynamics is None and mass_action is not None:
            custom_dynamics = not mass_action        
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
        self._update_rates()
        if not membrane._geometry.is_area():
            raise Exception('must specify a membrane not a volume for the boundary')
        self._regions = [membrane]
        #self._update_indices()
        rxd._register_reaction(self)


    def _update_rates(self):
        lhs = self._scheme._lhs._items
        rhs = self._scheme._rhs._items
        rate_f = self._original_rate_f
        rate_b = self._original_rate_b
        if not self._custom_dynamics:
            for k, v in zip(lhs.keys(), lhs.values()):
                if v == 1:
                    rate_f = rate_f * k
                else:
                    rate_f = rate_f * k ** v
        if rate_b is not None:
            if self._dir in ('<', '>'):
                raise Exception('unidirectional Reaction can have only one rate constant')
            if not self._custom_dynamics:
                for k, v in zip(rhs.keys(), rhs.values()):
                    if v == 1:
                        rate_b = rate_b * k
                    else:
                        rate_b = rate_b * k ** v
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
        self._memb_scales = -area_ratios * FARADAY / (10000 * molecules_per_mM_um3)    
        

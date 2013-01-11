import rxd
import node
import numpy
import weakref

# converting from mM um^3 to molecules
# = 6.02214129e23 * 1000. / 1.e18 / 1000
# = avogadro * (L / m^3) * (m^3 / um^3) * (mM / M)
# value for avogardro's constant from NIST webpage, accessed 25 April 2012:
# http://physics.nist.gov/cgi-bin/cuu/Value?na
molecules_per_mM_um3 = 602214.129

def ref_list_with_mult(obj):
    result = []
    for i, p in zip(obj.keys(), obj.values()):
        w = weakref.ref(i)
        result += [w] * p
    return result


class GeneralizedReaction:
    """an abstract class, parent of Rate, Reaction, MultiCompartmentReaction"""

    def __del__(self):
        rxd._unregister_reaction(self)

    def _setup_membrane_fluxes(self, sec_list, x_list):
        if not self._membrane_flux: return

        # locate the regions containing all species (including the one that changes)
        if all(sptr() for sptr in self._sources) and all(dptr() for dptr in self._dests):
            active_regions = [r for r in self._regions if all(sptr()._indices(r) for sptr in self._sources + self._dests)]
        else:
            active_regions = []
            
        for r in active_regions:
            for sec in r._secs:
                for i in xrange(sec.nseg):
                    sec_list.append(sec=sec)
                    x_list.append((i + 0.5) / sec.nseg)

        self._do_memb_scales()
        
    def _get_args(self, states):
        args = []
        for sptr in self._involved_species:
            s = sptr()
            if not s:
                return None
            args.append(states[self._indices_dict[s]])
        return args
        
    def _update_indices(self):
        # this is called anytime the geometry changes as well as at init
        
        self._indices_dict = {}
        
        # locate the regions containing all species (including the one that changes)
        if all(sptr() for sptr in self._sources) and all(dptr() for dptr in self._dests):
            active_regions = [r for r in self._regions if all(sptr().indices(r) for sptr in self._sources + self._dests)]
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
        sources_indices = [sum([sptr().indices(r) for r in active_regions], []) for sptr in self._sources]
        dests_indices = [sum([dptr().indices(r) for r in active_regions], []) for dptr in self._dests]
        self._indices = sources_indices + dests_indices
        volumes, surface_area, diffs = node._get_data()
        #self._mult = [list(-1. / volumes[sources_indices]) + list(1. / volumes[dests_indices])]
        if self._trans_membrane and active_regions:
            # note that this assumes (as is currently enforced) that if trans-membrane then only one region
            # TODO: verify the areas and volumes are in the same order!
            areas = numpy.array(sum([list(self._regions[0]._geometry.volumes1d(sec) for sec in self._regions[0].secs)], []))
            if not self._scale_by_area:
                areas = numpy.ones(len(areas))
            self._mult = list(-areas / volumes[sources_indices] / molecules_per_mM_um3) + list(areas / volumes[dests_indices])
            self._areas = areas
        else:
            self._mult = list([-1.] * len(v) for v in sources_indices) + list([1.] * len(v) for v in dests_indices)


    def _get_memb_flux(self, states):
        if self._membrane_flux:
            # TODO: refactor the inside of _evaluate so can construct args in a separate function and just get self._rate() result
            rates = self._evaluate(states)[2]
            return self._memb_scales * rates
        else:
            return []

    

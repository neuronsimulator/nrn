from . import rxd, node, rxdmath
import numpy
import weakref
import itertools
import scipy.sparse
import itertools

_weakref_ref = weakref.ref

# aliases to avoid repeatedly doing multiple hash-table lookups
_itertools_chain = itertools.chain
_numpy_array = numpy.array
_scipy_sparse_coo_matrix = scipy.sparse.coo_matrix

# converting from mM um^3 to molecules
# = 6.02214129e23 * 1000. / 1.e18 / 1000
# = avogadro * (L / m^3) * (m^3 / um^3) * (mM / M)
# value for avogardro's constant from NIST webpage, accessed 25 April 2012:
# http://physics.nist.gov/cgi-bin/cuu/Value?na
molecules_per_mM_um3 = 602214.129

def ref_list_with_mult(obj):
    result = []
    for i, p in zip(list(obj.keys()), list(obj.values())):
        w = _weakref_ref(i)
        result += [w] * p
    return result


def get_scheme_rate1_rate2_regions_custom_dynamics_mass_action(args, kwargs):
    """Parse the arguments to a rxd.Reaction or rxd.MultiCompartmentReaction.
    
    There are four valid options, two for historical
    compatibility, two for future support (these two are the ones
    described in the help)
    """
    if len(args) == 4:
        # bidirectional reaction
        # writing != instead of <> because Python 3 does not support <>
        scheme = (args[0] != args[1])
        rate1 = args[2]
        rate2 = args[3]
    elif len(args) == 3:
        # two possibilities which we can distinguish based on if the
        # first argument is an rxdmath._Reaction:
        # 1. with the new way, this would be reactants, products, and a
        #    forward rate
        # 2. with the old way, this is a bidirectional scheme
        if isinstance(args[0], rxdmath._Reaction):
            scheme = args[0]
            rate1 = args[1]
            rate2 = args[2]
        else:
            scheme = (args[0] > args[1])
            rate1 = args[2]
            rate2 = None
    elif len(args) == 2:
        # first argument must be a unidirectional rxdmath._Reaction
        # this is the old way and not included in the help because it
        # does not generalize to bidirectional reactions in Python 3
        # because of the missing <>
        scheme = args[0]
        if not isinstance(scheme, rxdmath._Reaction):
            raise RxDException('%r not a recognized reaction scheme' % self._scheme)
        rate1 = args[1]
        rate2 = None
    else:
        raise RxDException('Invalid number of arguments to rxd.Reaction')
        
    # keyword arguments
    # custom_dynamics is discouraged in favor of its antonym mass_action
    # (but internally we use custom_dynamics because of how originally
    # designed)
    regions = kwargs.get('regions')
    custom_dynamics = kwargs.get('custom_dynamics')
    mass_action = kwargs.get('mass_action')
    return scheme, rate1, rate2, regions, custom_dynamics, mass_action

class GeneralizedReaction(object):
    """an abstract class, parent of Rate, Reaction, MultiCompartmentReaction"""

    def __del__(self):
        rxd._unregister_reaction(self)

    def _setup_membrane_fluxes(self, node_indices, cur_map):
        # TODO: make sure this is redone whenever nseg changes
        if not self._membrane_flux: return

        # locate the regions containing all species (including the one that changes)
        if all(sptr() for sptr in self._sources) and all(dptr() for dptr in self._dests):
            active_regions = [r for r in self._regions if all(sptr().indices(r) for sptr in self._sources + self._dests)]
        else:
            active_regions = []
        node_indices_append = node_indices.append
        for r in active_regions:
            for sec in r._secs:
                for seg in sec:
                    node_indices_append(seg.node_index())

        self._do_memb_scales(cur_map)
        
    def _get_args(self, states):
        args = []
        args_append = args.append
        self_indices_dict = self._indices_dict
        for sptr in self._involved_species:
            s = sptr()
            if not s:
                return None
            args_append(states[self_indices_dict[s]])
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
            self._indices_dict[s] = list(_itertools_chain.from_iterable(s.indices(r) for r in active_regions))
        sources_indices = [list(_itertools_chain.from_iterable(sptr().indices(r) for r in active_regions)) for sptr in self._sources]
        dests_indices = [list(_itertools_chain.from_iterable(dptr().indices(r) for r in active_regions)) for dptr in self._dests]
        self._indices = sources_indices + dests_indices
        volumes, surface_area, diffs = node._get_data()
        #self._mult = [list(-1. / volumes[sources_indices]) + list(1. / volumes[dests_indices])]
        if self._trans_membrane and active_regions:
            # note that this assumes (as is currently enforced) that if trans-membrane then only one region
            # TODO: verify the areas and volumes are in the same order!
            areas = _numpy_array(list(_itertools_chain.from_iterable([list(self._regions[0]._geometry.volumes1d(sec)) for sec in self._regions[0].secs])))
            if not self._scale_by_area:
                areas = numpy.ones(len(areas))
            self._mult = [-areas / volumes[si] / molecules_per_mM_um3 for si in sources_indices] + [areas / volumes[di] / molecules_per_mM_um3 for di in dests_indices]
        else:
            self._mult = list(-1 for v in sources_indices) + list(1 for v in dests_indices)
        self._mult = _numpy_array(self._mult)
        self._update_jac_cache()


    def _evaluate(self, states):
        """returns: (list of lists (lol) of increase indices, lol of decr indices, list of changes)"""
        args = self._get_args(states)
        if args is None: return ([], [], [])
        return self._evaluate_args(args)

    def _evaluate_args(self, args):
        return (self._indices, self._mult, self._rate(*args))


    def _get_memb_flux(self, states):
        if self._membrane_flux:
            # TODO: refactor the inside of _evaluate so can construct args in a separate function and just get self._rate() result
            rates = self._evaluate(states)[2]
            return self._memb_scales * rates
        else:
            return []


    def _update_jac_cache(self):
        num_involved = len(self._involved_species)
        self._jac_rows = list(_itertools_chain(*[ind * num_involved for ind in self._indices]))
        num_ind = len(self._indices)
        self._jac_cols = list(_itertools_chain(*[self._indices_dict[s()] for s in self._involved_species])) * num_ind
        if self._trans_membrane:
            self._mult_extended = [sum([list(mul) * num_involved], []) for mul in self._mult]
            #self._mult_extended = [list(_itertools_chain.from_iterable(list(mul) * num_involved)) for mul in self._mult]
        else:
            self._mult_extended = self._mult
        


    def _jacobian_entries(self, states, multiply=1, dx=1.e-10):
        args = self._get_args(states)
        indices, mult, base_value = self._evaluate_args(args)
        mult = self._mult_extended
        derivs = []
        derivs_append = derivs.append
        for i, arg in enumerate(args):
            args[i] = arg + dx
            new_value = self._evaluate_args(args)[2]
            args[i] = arg
            derivs_append((new_value - base_value) / dx)
        derivs = _numpy_array(list(_itertools_chain(*derivs)))
        if self._trans_membrane:
            data = list(_itertools_chain(*[derivs * mul * multiply for mul in mult]))
            #data = derivs * mult * multiply
        else:
            data = list(_itertools_chain(*[derivs * mul * multiply for mul in mult]))
        return self._jac_rows, self._jac_cols, data
    
    def _jacobian(self, states, multiply=1, dx=1.e-10):
        rows, cols, data = self._jacobian_entries(states, multiply=multiply, dx=dx)
        n = len(states)
        jac = _scipy_sparse_coo_matrix((data, (rows, cols)), shape=(n, n))
        return jac

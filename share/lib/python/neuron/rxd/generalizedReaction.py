from . import node, rxdmath, constants
import numpy
import weakref
import itertools
from .rxdException import RxDException
from .species import xyz_by_index

_weakref_ref = weakref.ref

# aliases to avoid repeatedly doing multiple hash-table lookups
_itertools_chain = itertools.chain
_numpy_array = numpy.array


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
        scheme = args[0] != args[1]
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
            scheme = args[0] > args[1]
            rate1 = args[2]
            rate2 = None
    elif len(args) == 2:
        # first argument must be a unidirectional rxdmath._Reaction
        # this is the old way and not included in the help because it
        # does not generalize to bidirectional reactions in Python 3
        # because of the missing <>
        scheme = args[0]
        if not isinstance(scheme, rxdmath._Reaction):
            raise RxDException("%r not a recognized reaction scheme" % scheme)
        rate1 = args[1]
        rate2 = None
    else:
        raise RxDException("Invalid number of arguments to rxd.Reaction")

    # keyword arguments
    # custom_dynamics is discouraged in favor of its antonym mass_action
    # (but internally we use custom_dynamics because of how originally
    # designed)
    regions = kwargs.get("regions")
    custom_dynamics = kwargs.get("custom_dynamics")
    mass_action = kwargs.get("mass_action")
    return scheme, rate1, rate2, regions, custom_dynamics, mass_action


class GeneralizedReaction(object):
    """an abstract class, parent of Rate, Reaction, MultiCompartmentReaction"""

    def __del__(self):
        from .rxd import _unregister_reaction

        _unregister_reaction(self)

    def _setup_membrane_fluxes(self, node_indices, cur_map):
        # TODO: make sure this is redone whenever nseg changes
        if not self._membrane_flux:
            return

        from . import species

        # locate the regions containing all species (including the one that changes)
        # if all(sptr() for sptr in sources) and all(dptr() for dptr in dests):
        #    active_regions = [r for r in self._regions if all(sptr().indices(r) for sptr in sources + dests)]
        # else:
        #    active_regions = []
        node_indices_append = node_indices.append
        for r in self._active_regions:
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
        from . import species

        # Default values
        self._indices_dict = {}
        self._indices = []
        self._mult = [1]
        self._mult_extended = self._mult
        active_secs = None

        # filter regions with no sections
        def spfilter(sp):
            if not sp():
                return False
            regs = (
                sp()._regions if isinstance(sp(), species.Species) else [sp()._region()]
            )
            for r in regs:
                if r and (any(r._secs1d) or any(r._secs3d)):
                    break
            else:
                return False
            return True

        sources = list(
            filter(
                spfilter,
                [
                    r
                    for r in self._sources
                    if not isinstance(r(), species.SpeciesOnExtracellular)
                ],
            )
        )
        dests = list(
            filter(
                spfilter,
                [
                    r
                    for r in self._dests
                    if not isinstance(r(), species.SpeciesOnExtracellular)
                ],
            )
        )
        sources_ecs = [
            r for r in self._sources if isinstance(r(), species.SpeciesOnExtracellular)
        ]
        dests_ecs = [
            r for r in self._dests if isinstance(r(), species.SpeciesOnExtracellular)
        ]

        sp_regions = None
        if self._trans_membrane and (
            sources or dests
        ):  # assume sources share common regions and destinations share common regions
            sp_regions = list(
                {sptr()._region() for sptr in sources}.union(
                    {sptr()._region() for sptr in dests}
                )
            )
        elif sources and dests:
            sp_regions = list(
                set.intersection(
                    *[
                        set(sptr()._regions)
                        if isinstance(sptr(), species.Species)
                        else {sptr()._region()}
                        for sptr in sources + dests
                    ]
                )
            )

        # The reactants do not share a common region
        if not sp_regions:
            active_regions = [
                s()._extracellular()._region for s in sources_ecs + dests_ecs if s()
            ]
            # if a region is specified the reaction should only take place there
            if self._regions != [None]:
                self._active_regions = self._regions
            # alternatively if SpeciesOnExtracellular were specified the
            # reaction should only take place on those extracellular regions
            elif active_regions:
                self._active_regions = active_regions

            if hasattr(self, "_active_regions"):
                for reg in self._active_regions:
                    if not hasattr(reg, "_secs1d") or (
                        any(reg._secs1d) or any(reg._secs3d)
                    ):
                        break
                else:
                    if not sources_ecs or not dests_ecs:
                        return

            # if neither were specified don't set the '_has_regions' attribute
            # so the reaction takes place everywhere the species is defined
            for sptr in self._involved_species:
                self._indices_dict[sptr()] = []
            # Setup for extracellular
            self._mult = list(-1 for v in self._sources) + list(1 for v in self._dests)
            self._mult = _numpy_array(self._mult)
            return

        from .multiCompartmentReaction import MultiCompartmentReaction

        # locate the regions containing all species (including the one that changes)
        # (we do not need every species to be on the same region for multi compartment reactions)
        if all(sptr() for sptr in sources) and all(dptr() for dptr in dests):
            active_regions = [
                r
                for r in self._regions
                if all(sptr().indices(r) for sptr in sources + dests)
            ]
        else:
            active_regions = []
        # MultiCompartmentReactions do not require involved species to share common regions
        if not isinstance(self, MultiCompartmentReaction):
            for sptr in self._involved_species:
                s = sptr()
                if s and not isinstance(s, species.SpeciesOnExtracellular):
                    for r in self._regions:
                        if r in active_regions and not s.indices(r):
                            del active_regions[active_regions.index(r)]
                elif s and isinstance(s, species.SpeciesOnExtracellular):
                    r = s._extracellular()._region
                    if r in active_regions:
                        del active_regions[active_regions.index(r)]
                else:
                    active_regions = []

        def intersection(los):
            if los:
                return set.intersection(*los)
            return None

        # If we haven't identified active_regions -- use the regions where all species are defined
        if len(active_regions) == 0 or active_regions == [None]:
            if self._trans_membrane:
                src_regions = intersection(
                    [
                        set(sptr()._regions)
                        if isinstance(sptr(), species.Species)
                        else {sptr()._region()}
                        for sptr in sources
                    ]
                )
                if not src_regions:
                    raise RxDException(
                        f"Error in {self}. The source species do not share a common region"
                    )
                src_sections = intersection(
                    [set(reg.secs) for reg in src_regions if reg is not None]
                )
                dest_regions = intersection(
                    [
                        set(sptr()._regions)
                        if isinstance(sptr(), species.Species)
                        else {sptr()._region()}
                        for sptr in dests
                    ]
                )
                if not dest_regions:
                    raise RxDException(
                        f"Error in {self}. The destination species do not share a common region"
                    )
                dest_sections = intersection(
                    [set(reg.secs) for reg in dest_regions if reg is not None]
                )
                active_regions = set.union(src_regions, dest_regions)
                active_secs = set.union(src_sections, dest_sections)
            else:
                active_regions = list(
                    intersection(
                        [
                            set(sptr()._regions)
                            if isinstance(sptr(), species.Species)
                            else {sptr()._region()}
                            for sptr in sources + dests
                        ]
                    )
                )
                if not active_regions:
                    raise RxDException(
                        f"Error in {self}. The species do not share a common region"
                    )
                active_secs = set.intersection(
                    *[set(reg.secs) for reg in active_regions if reg]
                )
        else:
            active_secs = set.intersection(
                *[set(reg.secs) for reg in active_regions if reg]
            )

        self._active_regions = active_regions

        if isinstance(self, MultiCompartmentReaction):
            sources = [
                r
                for r in self._sources
                if not isinstance(r(), species.SpeciesOnExtracellular)
            ]
            dests = [
                r
                for r in self._dests
                if not isinstance(r(), species.SpeciesOnExtracellular)
            ]

            # flux occurs on sections which have both source, destination and membrane
            active_secs_list = list(self._regions[0]._secs1d)
            for sp in sources + dests:
                if sp() and sp()._region():
                    active_secs_list = [
                        sec for sec in active_secs_list if sec in sp()._region()._secs1d
                    ]
        else:
            active_secs_list = [
                sec
                for reg in active_regions
                if reg
                for sec in reg.secs
                if sec in active_secs
            ]

        # store the indices
        for sptr in self._involved_species:
            s = sptr()
            if not isinstance(s, species.SpeciesOnExtracellular):
                self._indices_dict[s] = s.indices(active_regions, active_secs)
        sources_indices = [
            sptr().indices(active_regions, active_secs) for sptr in sources
        ]
        dests_indices = [dptr().indices(active_regions, active_secs) for dptr in dests]
        self._indices = sources_indices + dests_indices
        volumes, surface_area, diffs = node._get_data()
        # self._mult = [list(-1. / volumes[sources_indices]) + list(1. / volumes[dests_indices])]
        if self._trans_membrane and active_regions:
            # note that this assumes (as is currently enforced) that if trans-membrane then only one region

            molecules_per_mM_um3 = constants.molecules_per_mM_um3()

            # TODO: verify the areas and volumes are in the same order!
            areas = _numpy_array(
                list(
                    _itertools_chain.from_iterable(
                        [
                            list(self._regions[0]._geometry.volumes1d(sec))
                            for sec in active_secs_list
                        ]
                    )
                )
            )
            if not self._scale_by_area:
                areas = numpy.ones(len(areas))
            if not sources_ecs and not dests_ecs:
                self._mult = [
                    -areas / volumes[si] / molecules_per_mM_um3
                    for si in sources_indices
                ] + [areas / volumes[di] / molecules_per_mM_um3 for di in dests_indices]
            # TODO: check for multicompartment reaction within the ECS
            elif sources_ecs and not dests_ecs:
                self._mult = [
                    -areas
                    / (
                        numpy.prod(s()._extracellular()._dx)
                        * s().alpha_by_location(xyz_by_index(di))
                    )
                    / molecules_per_mM_um3
                    for s, di in zip(sources_ecs, dests_indices)
                ] + [areas / volumes[di] / molecules_per_mM_um3 for di in dests_indices]
            elif not sources_ecs and dests_ecs:
                self._mult = [
                    -areas / volumes[si] / molecules_per_mM_um3
                    for si in sources_indices
                ] + [
                    areas
                    / (
                        numpy.prod(s()._extracellular()._dx)
                        * s().alpha_by_location(xyz_by_index(si))
                    )
                    / molecules_per_mM_um3
                    for s, si in zip(dests_ecs, sources_indices)
                ]

            else:
                # If both the source & destination are in the ECS, they should use a reaction
                # not a multicompartment reaction
                RxDException(
                    "An extracellular source and destination is not possible with a multi-compartment reaction."
                )
        else:
            self._mult = list(-1 for v in sources_indices) + list(
                1 for v in dests_indices
            )
        self._mult = _numpy_array(self._mult)
        self._update_jac_cache()

    def _evaluate(self, states):
        """returns: (list of lists (lol) of increase indices, lol of decr indices, list of changes)"""
        args = self._get_args(states)
        if args is None:
            return ([], [], [])
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
        self._mult_extended = self._mult

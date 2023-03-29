from . import rxdmath, rxd, initializer
from .generalizedReaction import (
    GeneralizedReaction,
    ref_list_with_mult,
    get_scheme_rate1_rate2_regions_custom_dynamics_mass_action,
)
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
        (
            scheme,
            rate1,
            rate2,
            regions,
            custom_dynamics,
            mass_action,
        ) = get_scheme_rate1_rate2_regions_custom_dynamics_mass_action(args, kwargs)

        # TODO: verify schemes use weakrefs
        self._scheme = scheme
        if custom_dynamics is not None and mass_action is not None:
            raise RxDException("Cannot specify both custom_dynamics and mass_action.")
        elif custom_dynamics is None and mass_action is None:
            custom_dynamics = False
        elif custom_dynamics is None and mass_action is not None:
            custom_dynamics = not mass_action
        self._custom_dynamics = custom_dynamics
        if scheme._dir == "<":
            rate_f, rate_b = 0, rate1
            if rate2 is not None:
                raise RxDException(
                    "unidirectional Reaction can have only one rate constant"
                )
        elif scheme._dir == "<>":
            rate_f, rate_b = rate1, rate2
            if rate2 is None:
                raise RxDException("bidirectional Reaction needs two rate constants")
        elif scheme._dir == ">":
            rate_f, rate_b = rate1, 0
            if rate2 is not None:
                raise RxDException(
                    "unidirectional Reaction can have only one rate constant"
                )
        else:
            raise RxDException(f"unknown reaction scheme direction: {scheme._dir}")

        self._original_rate_f = rate_f
        self._original_rate_b = rate_b
        for ar in [scheme, rate_f, rate_b]:
            try:
                if ar._voltage_dependent:
                    self._voltage_dependent = True
                    break
            except AttributeError:
                pass
        else:
            self._voltage_dependent = False
        self._membrane_flux = False
        self._dir = scheme._dir
        self._custom_dynamics = custom_dynamics

        self._trans_membrane = False
        if not hasattr(regions, "__len__"):
            regions = [regions]
        self._regions = regions
        rxd._register_reaction(self)

        # initialize self if the rest of rxd is already initialized
        if initializer.is_initialized():
            self._do_init()

    def _do_init(self):
        self._update_rates()
        self._update_indices()

    def _update_rates(self):
        lhs = self._scheme._lhs._items
        rhs = self._scheme._rhs._items
        if self._dir == "<":
            # TODO: remove this limitation (probably means doing with rate_b what done with rate_f and making sure _sources and _dests are correct
            raise RxDException(
                "pure reverse reaction currently not supported; reformulate as a forward reaction"
            )

        rate_f = rxdmath._ensure_arithmeticed(self._original_rate_f)
        rate_b = rxdmath._ensure_arithmeticed(self._original_rate_b)

        if not self._custom_dynamics:
            for k, v in zip(list(lhs.keys()), list(lhs.values())):
                if v == 1:
                    rate_f *= k
                else:
                    rate_f *= k**v
            if self._dir == "<>":
                for k, v in rhs.items():
                    if v == 1:
                        rate_b *= k
                    else:
                        rate_b *= k**v
        rate = rate_f - rate_b
        self._rate_arithmeticed = rate

        self._sources = ref_list_with_mult(lhs)
        self._dests = ref_list_with_mult(rhs)

        # Check to if it is an extracellular reaction
        from . import region, species

        # Was an ECS region was passed to to the constructor
        ecs_region = [r for r in self._regions if isinstance(r, region.Extracellular)]

        # Are any of of the sources or destinations passed to the constructor extracellular
        if not ecs_region:
            ecs_species = [
                s()
                for s in self._sources + self._dests
                if isinstance(s(), species.SpeciesOnExtracellular)
                or isinstance(s(), species._ExtracellularSpecies)
            ]
            if ecs_species:
                ecs_region = (
                    [ecs_species[0]._region]
                    if isinstance(ecs_species[0], species._ExtracellularSpecies)
                    else [ecs_species[0]._extracellular()._region]
                )

        # Are any of of the sources or destinations passed to the constructor defined on the ECS
        if not ecs_region:
            sps = [
                s()
                for s in self._sources + self._dests
                if isinstance(s(), species.Species)
            ]
            # only have an ecs reaction if all the species are defined on the ecs
            if sps and all(s._extracellular_instances for s in sps):
                # take an intersection of all the extracellular regions
                ecs_region = list(sps[0]._extracellular_instances.keys())
                for s in sps:
                    ecs_region = [
                        r for r in s._extracellular_instances.keys() if r in ecs_region
                    ]
        if ecs_region:
            self._rate_ecs, self._involved_species_ecs = rxdmath._compile(
                rate, ecs_region
            )

        # if a region is specified -- use it
        if self._regions and self._regions != [None]:
            self._react_regions = [
                r for r in self._regions if any(r._secs1d) or any(r._secs3d)
            ]
        else:
            # else find the common regions shared by all sources and  destinations
            self._react_regions = []
            regs = []
            for sptr in self._sources + self._dests:
                s = sptr() if isinstance(sptr(), species.Species) else sptr()._species()
                regs.append(
                    set([r for r in s._regions if any(r._secs1d) or any(r._secs3d)])
                )
            self._react_regions = list(set.intersection(*regs))
        if self._react_regions:
            self._rate, self._involved_species = rxdmath._compile(
                rate, self._react_regions
            )
        else:
            self._rate, self._involved_species = None, []

        # Species are in at most one region
        trans_membrane = (
            len(
                {
                    s()._region()
                    for s in self._sources + self._dests + self._involved_species
                    if isinstance(s(), species.SpeciesOnRegion)
                }
            )
            + len(
                {
                    s()._extracellular()._region
                    for s in self._sources + self._dests + self._involved_species
                    if isinstance(s(), species.SpeciesOnExtracellular)
                }
            )
            > 1
        )

        if trans_membrane:
            raise RxDException(
                "Reaction does not support multi-compartment dynamics. Use MultiCompartmentReaction."
            )

        # Recompile all the reactions in C
        if hasattr(self, "_mult"):
            rxd._compile_reactions()

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
        if self._dir not in ("<>", ">"):
            raise RxDException("no forward reaction in reaction scheme")
        self._original_rate_f = value
        self._update_rates()

    @b_rate.setter
    def b_rate(self, value):
        if self._dir not in ("<>", "<"):
            raise RxDException("no backward reaction in reaction scheme")
        self._original_rate_b = value
        self._update_rates()

    def __repr__(self):
        short_f = (
            self._original_rate_f._short_repr()
            if hasattr(self._original_rate_f, "_short_repr")
            else self._original_rate_f
        )
        short_b = (
            self._original_rate_b._short_repr()
            if hasattr(self._original_rate_b, "_short_repr")
            else self._original_rate_b
        )

        if len(self._regions) != 1 or self._regions[0] is not None:
            regions_short = (
                "[" + ", ".join(r._short_repr() for r in self._regions) + "]"
            )
            return "Reaction(%s, %s, rate_b=%s, regions=%s, custom_dynamics=%r)" % (
                self._scheme,
                short_f,
                short_b,
                regions_short,
                self._custom_dynamics,
            )
        else:
            return "Reaction(%s, %s, rate_b=%s, custom_dynamics=%r)" % (
                self._scheme,
                short_f,
                short_b,
                self._custom_dynamics,
            )

    def _do_memb_scales(self):
        # nothing to do since NEVER a membrane flux
        pass

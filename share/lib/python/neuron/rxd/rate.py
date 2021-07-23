from .rxdException import RxDException
import weakref
from . import species, rxdmath, rxd, initializer
import numpy
from .rangevar import RangeVar
import itertools
import warnings
from .generalizedReaction import GeneralizedReaction

# aliases to avoid repeatedly doing multiple hash-table lookups
_itertools_chain = itertools.chain


class Rate(GeneralizedReaction):
    """Declare a contribution to the rate of change of a species or other state variable.

    Example:

        constant_production = rxd.Rate(protein, k)

    If this was the only contribution to protein dynamics and there was no
    diffusion, the above would be equivalent to:

        dprotein/dt = k

    If there are multiple rxd.Rate objects (or an rxd.Reaction, etc) acting on
    the same species, then their effects are summed.
    """

    def __init__(self, species, rate, regions=None, membrane_flux=False):
        """create a rate of change for a species on a given region or set of regions

        if regions is None, then does it on all regions"""
        self._species = weakref.ref(species)
        self._original_rate = rate
        from . import region

        if not hasattr(regions, "__len__"):
            if regions is not None and not isinstance(regions, region.Extracellular):
                regions = [regions]
            else:
                if hasattr(species, "_extracellular"):
                    regions = [species._extracellular()._region]
                elif hasattr(species, "region"):
                    regions = [species._region()]
                else:
                    regions = []
                    if hasattr(species, "_extracellular_regions"):
                        regions += species._extracellular_regions
                    if hasattr(species, "_regions"):
                        regions += species._regions
                    if not regions:
                        regions = [None]
        else:
            regions = [reg for reg in regions]
        self._regions = regions
        self._membrane_flux = membrane_flux
        if membrane_flux not in (True, False):
            raise RxDException("membrane_flux must be either True or False")
        if membrane_flux and regions is None:
            # TODO: rename regions to region?
            raise RxDException(
                "if membrane_flux then must specify the (unique) membrane regions"
            )
        self._trans_membrane = False
        rxd._register_reaction(self)

        # be careful, this could keep states alive
        self._original_rate = rate

        try:
            self._voltage_dependent = rate._voltage_dependent
        except AttributeError:
            self._voltage_dependent = False

        # initialize self if the rest of rxd is already initialized
        if initializer.is_initialized():
            self._do_init()

    def _do_init(self):
        from . import region, species

        rate = self._original_rate
        if not isinstance(rate, RangeVar):
            regions = []
            if self._regions and self._regions != [None]:
                regions = self._regions
            elif hasattr(self._species(), "_regions"):
                regions = self._species()._regions
            elif hasattr(self._species(), "_region"):
                regions = [self._species()._region()]
            regions = [
                r
                for r in regions
                if isinstance(r, region.Extracellular) or (r._secs1d or r._secs3d)
            ]
            if regions:
                self._rate, self._involved_species = rxdmath._compile(rate, regions)
            else:
                self._rate, self._involved_species = {}, []
        else:
            self._involved_species = self._species
        self._update_indices()

        # Check to if it is an extracellular reaction
        # Was an ECS region was passed to to the constructor
        ecs_region = [r for r in self._regions if isinstance(r, region.Extracellular)]
        # ecs_region = ecs_region[0] if len(ecs_region) > 0 else None
        # commented above out line because we need to pass all ecs_regions in.
        # TODO review this code below...I am very tired. Also did this in reaction.py
        ecs_region = ecs_region if len(ecs_region) > 0 else None
        if ecs_region:
            self._ecs_regions = ecs_region
        # Is the species passed to the constructor extracellular
        if not ecs_region:
            if isinstance(self._species(), species.SpeciesOnExtracellular):
                ecs_region = self._species()._extracellular()
            elif isinstance(self._species(), species._ExtracellularSpecies):
                ecs_region = self._species()._region

        # Is the species passed to the constructor defined on the ECS
        if not ecs_region:
            if not isinstance(self._species(), species.SpeciesOnRegion):
                sp = self._species()
                if sp and sp._extracellular_instances:
                    self._ecs_regions = [
                        key for key in sp._extracellular_instances.keys()
                    ]

        if hasattr(self, "_ecs_regions"):
            self._rate_ecs, self._involved_species_ecs = rxdmath._compile(
                rate, self._ecs_regions
            )
            for sp in self._involved_species_ecs:
                if ecs_region and isinstance(sp(), species.SpeciesOnRegion):
                    raise RxDException(
                        "Error Rate %r: an extracellular rate can not depend on a SpeciesOnRegion"
                        % self._original_rate
                    )

    def __repr__(self):
        short_rate = (
            self._original_rate._short_repr()
            if hasattr(self._original_rate, "_short_repr")
            else self._original_rate
        )
        if len(self._regions) != 1 or self._regions[0] is not None:
            regions_short = (
                "[" + ", ".join(r._short_repr() for r in self._regions) + "]"
            )
            return "Rate(%s, %s, regions=%s, membrane_flux=%r)" % (
                self._species()._short_repr(),
                short_rate,
                regions_short,
                self._membrane_flux,
            )
        else:
            return "Rate(%s, %s, membrane_flux=%r)" % (
                self._species()._short_repr(),
                short_rate,
                self._membrane_flux,
            )

    def _rate_from_rangevar(self, *args):
        return self._original_rate._rangevar_vec()

    def _update_indices(self):
        # this is called anytime the geometry changes as well as at init
        # TODO: is the above statement true?

        # Default values
        self._indices_dict = {}
        self._indices = []
        self._jac_rows = []
        self._jac_cols = []
        self._mult = [1]
        self._mult_extended = self._mult
        from . import region

        if (
            not self._species()
            or isinstance(self._species(), species.SpeciesOnExtracellular)
            or all([isinstance(r, region.Extracellular) for r in self._regions])
        ):
            if self._regions != [None]:
                self._active_regions = self._regions
            elif self._species():
                if isinstance(self._species(), species.SpeciesOnExtracellular):
                    self._active_regions = [self._species()._extracellular()._region]
            if hasattr(self, "_involved_species"):
                for sptr in self._involved_species:
                    self._indices_dict[sptr()] = []
            return

        active_secs = None

        # locate the regions containing all species (including the one that changes)
        active_regions = list(
            set.intersection(
                *[
                    set(
                        sptr()._regions + sptr()._extracellular_regions
                        if isinstance(sptr(), species.Species)
                        else [
                            sptr()._region()
                            if isinstance(sptr(), species.SpeciesOnRegion)
                            else sptr()._extracellular()
                        ]
                    )
                    for sptr in list(self._involved_species) + [self._species]
                ]
            )
        )
        sp_regions = (
            self._species()._regions + self._species()._extracellular_regions
            if isinstance(self._species(), species.Species)
            else [
                self._species()._region()
                if isinstance(self._species(), species.SpeciesOnRegion)
                else self._species()._extracellular()
            ]
        )
        actr = sp_regions
        if self._regions != [None]:
            specified_regions = list(
                set.intersection(set(self._regions), set(sp_regions))
            )
            if specified_regions:
                actr = specified_regions
                active_regions = list(set.intersection(set(active_regions), set(actr)))
            else:
                warnings.warn(
                    "Error in rate %r\nThe regions specified %s are not appropriate regions %s will be used instead."
                    % (
                        self,
                        [r._name for r in self._regions],
                        [r._name for r in self._species()._regions],
                    )
                )

        # Note: This finds the sections where the all involved species exists
        # e.g. for Rate(A, B+C) if A sections [1,2,3] and B is on sections [1,2] and C is on sections [2,3]
        # The Rate will only effect A on section 2 (rather than have 3 different rates)
        if not active_regions:  # They do not share a common region
            if any(
                isinstance(sptr(), species.Species)
                for sptr in list(self._involved_species) + [self._species]
            ):
                for sptr in self._involved_species:
                    self._indices_dict[sptr()] = []
                return
            active_1d_secs = list(
                set.intersection(
                    *[set(reg._secs1d) for reg in actr if hasattr(reg, "_secs1d")]
                )
            )
            active_3d_secs = list(
                set.intersection(
                    *[set(reg._secs3d) for reg in actr if hasattr(reg, "_secs3d")]
                )
            )
            active_regions = actr
            # if there are multiple regions on a segment for an involved species the rate is ambiguous
            for sptr in self._involved_species:
                s = sptr()
                indices = [list(s.indices(secs={sec})) for sec in active_1d_secs]
                if not all(
                    rcount == sec.nseg or rcount == 0
                    for rcount, sec in zip(
                        [len(ind) for ind in indices], active_1d_secs
                    )
                ):
                    raise RxDException(
                        "Error in rate %r, the species do not share a common region"
                        % self
                    )
                # remove sections where species is absent
                active_1d_secs = {
                    sec
                    for sec, ind in zip(active_1d_secs, indices)
                    if len(ind) == sec.nseg
                }
            # Repeated with the trimmed active_1d_secs and store the indices
            if active_1d_secs:
                for sptr in self._involved_species:
                    s = sptr()
                    indices = [list(s.indices(secs={sec})) for sec in active_1d_secs]
                    self._indices_dict[s] = list(
                        _itertools_chain.from_iterable(indices)
                    )
                    self._indices = [self._species().indices(actr, active_1d_secs)]
            elif active_3d_secs:
                pass
                # Account for hybrid stuff as well
                # print("we have an active 3d sec")
            else:
                raise RxDException(
                    "Error in rate %r, the species do not share a common section" % self
                )
        else:
            active_secs = [
                set(reg.secs) for reg in active_regions if hasattr(reg, "secs")
            ]
            if active_secs:
                active_secs = set.union(*active_secs)
            else:
                active_secs = set()
            # store the indices
            for sptr in self._involved_species:
                s = sptr()
                self._indices_dict[s] = s.indices(active_regions, active_secs)

            self._indices = [self._species().indices(active_regions, active_secs)]
        self._active_regions = active_regions

        if isinstance(self._original_rate, RangeVar):
            nodes = []
            for sptr in self._involved_species:
                s = sptr()
                for r in active_regions:
                    if r is None:
                        nodes += s.nodes
                    else:
                        nodes += s[r].nodes
            self._original_rate._init_ptr_vectors(nodes)
            self._indices = [self._original_rate._locs]
            self._rate = self._rate_from_rangevar
            self._get_args = lambda ignore: []
        else:
            self._update_jac_cache()

    def _do_memb_scales(self):
        # TODO: does anyone still call this?
        # TODO: update self._memb_scales (this is just a dummy value to make things run)
        self._memb_scales = 1

    def _get_memb_flux(self, states):
        if self._membrane_flux:
            # raise RxDException('membrane flux due to rxd.Rate objects not yet supported')
            # TODO: refactor the inside of _evaluate so can construct args in a separate function and just get self._rate() result
            rates = self._evaluate(states)[2]
            return self._memb_scales * rates
        else:
            return []

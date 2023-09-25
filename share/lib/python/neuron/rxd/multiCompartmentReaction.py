import weakref
from . import rxdmath, rxd, species, initializer
import numpy
from .generalizedReaction import (
    GeneralizedReaction,
    get_scheme_rate1_rate2_regions_custom_dynamics_mass_action,
)
from .constants import molecules_per_mM_um3
from neuron import h
import itertools
from .rxdException import RxDException


def _ref_list_with_mult(obj):
    result = []
    for i, p in zip(list(obj.keys()), list(obj.values())):
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
        (
            scheme,
            rate_f,
            rate_b,
            regions,
            custom_dynamics,
            mass_action,
        ) = get_scheme_rate1_rate2_regions_custom_dynamics_mass_action(args, kwargs)
        # additional keyword arguments
        membrane_flux = kwargs.get("membrane_flux", False)
        membrane = kwargs.get("membrane")
        border = kwargs.get("border")
        scale_by_area = kwargs.get("scale_by_area", True)

        if border is not None and membrane is not None:
            raise Exception("cannot specify both border and membrane; they are synoyms")
        if border is not None:
            membrane = border

        # TODO: verify schemes use weakrefs
        self._scheme = scheme
        self._scale_by_area = scale_by_area
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

        if custom_dynamics is not None and mass_action is not None:
            raise RxDException("Cannot specify both custom_dynamics and mass_action.")
        elif custom_dynamics is None and mass_action is None:
            custom_dynamics = False
        elif custom_dynamics is None and mass_action is not None:
            custom_dynamics = not mass_action
        self._custom_dynamics = custom_dynamics
        self._trans_membrane = True
        if membrane_flux not in (True, False):
            raise RxDException("membrane_flux must be either True or False")
        if membrane is None:
            raise RxDException("MultiCompartmentReaction requires a membrane parameter")
        if membrane_flux and species._has_3d:
            raise RxDException("membrane_flux not supported except in 1D")
        self._membrane_flux = membrane_flux
        if not isinstance(scheme, rxdmath._Reaction):
            raise RxDException("%r not a recognized reaction scheme" % self._scheme)
        self._dir = scheme._dir
        if not membrane._geometry.is_area():
            raise RxDException("must specify a membrane not a volume for the boundary")
        self._regions = [membrane]
        rxd._register_reaction(self)

        # initialize self if the rest of rxd is already initialized
        if initializer.is_initialized():
            self._do_init()
            self._update_indices()

    def _do_init(self):
        self._update_rates()

    def _update_rates(self):
        lhs = self._scheme._lhs._items
        rhs = self._scheme._rhs._items
        rate_f = self._original_rate_f
        rate_b = self._original_rate_b
        if not self._custom_dynamics:
            for k, v in zip(list(lhs.keys()), list(lhs.values())):
                if v == 1:
                    rate_f = rate_f * k
                else:
                    rate_f = rate_f * k**v
        if rate_b is not None:
            if self._dir in ("<", ">"):
                raise RxDException(
                    "unidirectional Reaction can have only one rate constant"
                )
            if not self._custom_dynamics:
                for k, v in zip(list(rhs.keys()), list(rhs.values())):
                    if v == 1:
                        rate_b = rate_b * k
                    else:
                        rate_b = rate_b * k**v
            rate = rate_f - rate_b
            self._sources = _ref_list_with_mult(lhs)
            self._dests = _ref_list_with_mult(rhs)
        elif self._dir == "<>":
            raise RxDException("bidirectional Reaction needs two rate constants")
        elif self._dir == ">":
            rate = rate_f
            self._sources = _ref_list_with_mult(lhs)
            self._dests = _ref_list_with_mult(rhs)
        elif self._dir == "<":
            self._sources = _ref_list_with_mult(rhs)
            self._dests = _ref_list_with_mult(lhs)
            rate = rate_f
        else:
            raise RxDException("unrecognized direction; should never happen")

        # check that the regions have sections
        for reg in self._regions:
            if any(reg._secs1d) or any(reg._secs3d):
                break
        else:
            return

        # check for 3D sections
        self._src3d = set()
        self._dst3d = set()
        self._mem3d = set(self._regions[0]._secs3d)
        sources = [
            s()._region()
            for s in self._sources
            if not isinstance(s(), species.SpeciesOnExtracellular)
        ]
        dests = [
            s()._region()
            for s in self._dests
            if not isinstance(s(), species.SpeciesOnExtracellular)
        ]
        for reg in sources:
            if any(reg._secs3d):
                self._src3d.update(reg._secs3d)
        for reg in dests:
            if any(reg._secs3d):
                self._dst3d.update(reg._secs3d)
        # if self._src3d.intersection(self._dst3d).intersection(self._mem3d):
        #    #Find all interacting voxels for each grid. Also build up the 2D array of index maps
        #    raise RxDException('Multicompartment reactions in 3D are not yet supported.')

        # check there are common 1D section between source and destination
        mem1d = set(self._regions[0]._secs1d)
        src1d = set()
        dst1d = set()
        for reg in sources:
            if any(reg._secs1d):
                src1d.update(reg._secs1d)
        for reg in dests:
            if any(reg._secs1d):
                dst1d.update(reg._secs1d)
        if sources:
            mem1d = mem1d.intersection(src1d)
        if dests:
            mem1d = mem1d.intersection(dst1d)
        if (src1d or dst1d) and not mem1d:
            msg = (
                "sources and destinations"
                if sources and dests
                else "sources"
                if sources
                else "destinations"
            )
            raise RxDException(
                "Multicompartment reactions the membrane and %s must share common sections. %r"
                % (msg, self)
            )

        self._changing_species = list(set(self._sources + self._dests))

        mem = self._regions[0]
        regs = [mem]
        # for sptr in self._changing_species:
        #    if isinstance(sptr(), species.Species):
        #        raise RxDException('must specify region for all involved species')
        #    elif hasattr(sptr(),'_extracellular'):
        #        regs.append(sptr()._extracellular()._region)
        #    else:
        #        regs.append(sptr()._region())
        self._rate, self._involved_species = rxdmath._compile(rate, regs)

    @property
    def f_rate(self):
        return self._original_rate_f

    @property
    def b_rate(self):
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

        return (
            "MultiCompartmentReaction(%r, %s, rate_b=%s, membrane=%s, custom_dynamics=%r, membrane_flux=%r, scale_by_area=%r)"
            % (
                self._scheme,
                short_f,
                short_b,
                self._regions[0]._short_repr(),
                self._custom_dynamics,
                self._membrane_flux,
                self._scale_by_area,
            )
        )

    def _do_memb_scales(self, cur_map):
        from . import species

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
        active_secs = list(self._regions[0]._secs1d)
        for sp in sources + dests:
            if sp() and sp()._region():
                active_secs = [
                    sec for sec in active_secs if sec in sp()._region()._secs1d
                ]

        if not self._scale_by_area:
            narea = sum([sec.nseg for sec in active_secs])
            areas = numpy.ones(narea)
        else:
            # TODO: simplify this expression
            areas = numpy.fromiter(
                itertools.chain.from_iterable(
                    list(
                        self._regions[0]._geometry.volumes1d(sec) for sec in active_secs
                    )
                ),
                dtype=float,
            )
        neuron_areas = []
        for sec in active_secs:
            neuron_areas += [seg.area() for seg in sec]
        neuron_areas = numpy.array(neuron_areas)
        # area_ratios is usually a vector of 1s
        area_ratios = areas / neuron_areas

        # still needs to be multiplied by the valence of each molecule
        self._memb_scales = -area_ratios * h.FARADAY / (10000 * molecules_per_mM_um3())

        # since self._memb_scales is only used to compute currents as seen by the rest of NEURON,
        # we only use NEURON's areas
        # self._memb_scales = volume * molecules_per_mM_um3 / areas

        if not self._membrane_flux:
            return
        # TODO: don't assume/require always inside/outside on one side...
        #       if no nrn_region specified, then (make so that) no contribution
        #       to membrane flux
        sources_ecs = [
            r for r in self._sources if isinstance(r(), species.SpeciesOnExtracellular)
        ]
        dests_ecs = [
            r for r in self._dests if isinstance(r(), species.SpeciesOnExtracellular)
        ]

        source_regions = [s()._region()._nrn_region for s in sources] + [
            "o" for s in sources_ecs
        ]
        dest_regions = [d()._region()._nrn_region for d in dests] + [
            "o" for d in dests_ecs
        ]
        # dereference the species to get the true species if it's actually a SpeciesOnRegion
        sources = [s()._species() for s in self._sources]
        dests = [d()._species() for d in self._dests]

        # TODO: make so don't need multiplicity (just do in one pass)
        smap = {"i": 1, None: 0, "o": -1}
        self._cur_charges = tuple(
            [
                smap[r] * s.charge
                for s, r in zip(sources, source_regions)
                if s.name is not None
            ]
            + [
                -1 * smap[r] * s.charge
                for s, r in zip(dests, dest_regions)
                if s.name is not None
            ]
        )
        self._net_charges = sum(self._cur_charges)
        self._cur_ptrs = []
        self._cur_mapped = []
        self._cur_mapped_ecs = []
        ecs_grids = dict()
        all_grids = []
        for sp in itertools.chain(self._sources, self._dests):
            s = sp()._species()
            if s.name is not None:
                for r in s.regions:
                    if isinstance(s[r], species.SpeciesOnExtracellular):
                        ecs_grids[s.name] = s[r]._extracellular()
                if s.name not in all_grids:
                    all_grids.append((s.name, s.charge))
        for sec in active_secs:
            for seg in sec:
                local_ptrs = []
                local_mapped = []
                local_mapped_ecs = []
                for spname, charge in all_grids:
                    # Check for extracellular regions
                    if charge != 0:
                        name = "_ref_i%s" % (spname)
                        local_ptrs.append(getattr(seg, name))
                        uberlocal_map = [None, None]
                        uberlocal_map_ecs = [None, None]
                        if spname + "i" in cur_map and cur_map[spname + "i"]:
                            uberlocal_map[0] = cur_map[spname + "i"][seg]
                        if spname + "o" in cur_map:
                            # Original rxd extracellular region
                            if seg in cur_map[spname + "o"]:
                                uberlocal_map[1] = cur_map[spname + "o"][seg]
                            elif spname in ecs_grids:  # Extracellular space
                                uberlocal_map_ecs[0] = ecs_grids[
                                    spname
                                ]._grid_id  # TODO: Just pass the grid_id once per species
                                uberlocal_map_ecs[1] = ecs_grids[spname].index_from_xyz(
                                    *species._xyz(seg)
                                )
                        local_mapped.append(uberlocal_map)
                        local_mapped_ecs.append(uberlocal_map_ecs)
                self._cur_ptrs.append(tuple(local_ptrs))
                self._cur_mapped.append(tuple(local_mapped))
                self._cur_mapped_ecs.append(local_mapped_ecs)

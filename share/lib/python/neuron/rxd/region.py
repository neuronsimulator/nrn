from .rxdException import RxDException
from neuron import h

try:
    from . import geometry3d
except:
    pass
import itertools
import numpy
from . import geometry as geo
import weakref
from . import initializer
from .geometry import FractionalVolume
import warnings
import math
import ctypes

_all_regions = []
_region_count = 0
_c_region_lookup = (
    None  # a global dictionary linking a region to a list of _c_regions it is a part of
)


def _sort_secs(secs):
    # sort the sections
    root_secs = h.SectionList()
    root_secs.allroots()
    all_sorted = h.SectionList()
    for root in root_secs:
        all_sorted.wholetree(sec=root)
    secs_names = dict([(sec.hoc_internal_name(), sec) for sec in secs])
    # for sec in secs:
    #    if sec.orientation():
    #        raise RxDException('still need to deal with backwards sections')
    secs = [
        secs_names[sec.hoc_internal_name()]
        for sec in all_sorted
        if sec.hoc_internal_name() in secs_names
    ]
    # return an empty list rather than an empty SectionList because
    # bool([]) == False
    # bool(h.SectionList([])) == True
    # secs are checked in some reactions to determine active regions
    return [] if secs == [] else h.SectionList(secs)


class _c_region:
    """
    The overlapping regions that are used to parse the relevant indices for JIT C reactions.
    regions - a set of regions that occur in the same sections
    """

    def __init__(self, regions):
        global _c_region_lookup
        self._regions = [weakref.ref(r) for r in regions]
        self._overlap = self._regions[0]()._secs1d
        self.num_regions = len(self._regions)
        self.num_species = 0
        self.num_params = 0
        self.num_ecs_species = 0
        self.num_ecs_params = 0
        self._ecs_react_species = []
        self._ecs_react_params = []
        self._react_species = []
        self._react_params = []
        self._react_regions = {}
        self._initialized = False
        self.location_index = None
        self.ecs_location_index = None
        self._ecs_species_ids = None
        self._ecs_params_ids = None
        self._voltage_dependent = False
        self._vptrs = None
        for rptr in self._regions:
            r = rptr()
            self._overlap = h.SectionList(
                [sec for sec in r._secs1d if sec in self._overlap]
            )
            if r in _c_region_lookup:
                _c_region_lookup[rptr].append(self)
            else:
                _c_region_lookup[rptr] = [self]

    def add_reaction(self, rptr, region):
        # for multicompartment reaction -- check all regions are present
        if rptr() and hasattr(rptr(), "_changing_species"):
            for sptr in rptr()._changing_species:
                if (
                    sptr()
                    and hasattr(sptr(), "_region")
                    and sptr()._region not in self._regions
                ):
                    return
        if rptr in self._react_regions:
            if region not in self._react_regions[rptr]:
                self._react_regions[rptr].append(region)
        else:
            self._react_regions[rptr] = [region]
        if not self._voltage_dependent:
            self._voltage_dependent = rptr()._voltage_dependent
        self._initialized = False

    def add_species(self, species_set):
        from .species import SpeciesOnRegion, Parameter, ParameterOnRegion

        for s in species_set:
            if isinstance(s, ParameterOnRegion):
                if s._species() and s._species not in self._react_params:
                    self._react_params.append(s._species)
            elif isinstance(s, Parameter):
                if s._species not in self._react_params:
                    self._react_params.append(s._species)
            elif isinstance(s, SpeciesOnRegion):
                if s._species() and s._species not in self._react_species:
                    self._react_species.append(s._species)
            elif weakref.ref(s) not in self._react_species:
                self._react_species.append(weakref.ref(s))
        self.num_params = len(self._react_params)
        self.num_species = len(self._react_species)
        self._initilized = False

    def add_ecs_species(self, species_set):
        from .species import ParameterOnExtracellular

        for s in species_set:
            sptr = s._extracellular
            if isinstance(s, ParameterOnExtracellular):
                if sptr not in self._ecs_react_params:
                    self._ecs_react_params.append(sptr)
            else:
                if sptr not in self._ecs_react_species:
                    self._ecs_react_species.append(sptr)
        self.num_ecs_params = len(self._ecs_react_params)
        self.num_ecs_species = len(self._ecs_react_species)
        self._initialized = False

    def get_ecs_index(self):
        if not self._initialized:
            self._initalize()
        if self.ecs_location_index is None:
            return numpy.ndarray(0, ctypes.c_int)
        else:
            return self.ecs_location_index.flatten()

    def get_state_index(self):
        if not self._initialized:
            self._initalize()
        return self.location_index.flatten()

    def get_ecs_species_ids(self):
        ret = numpy.ndarray(self.num_ecs_species + self.num_ecs_params, ctypes.c_int)
        if self.num_ecs_species + self.num_ecs_params > 0:
            for i in self._ecs_species_ids:
                ret[self._ecs_species_ids[i]] = i
            for i in self._ecs_params_ids:
                ret[self._ecs_params_ids[i] + self.num_ecs_species] = i
        return ret

    def _ecs_initalize(self):
        from . import species

        self.ecs_location_index = -numpy.ones(
            (self.num_ecs_species + self.num_ecs_params, self.num_segments),
            ctypes.c_int,
        )

        self._ecs_react_species.sort(
            key=lambda sp: sp()._extracellular()._grid_id
            if isinstance(sp(), species.SpeciesOnExtracellular)
            else sp()._grid_id
        )
        self._ecs_react_params.sort(
            key=lambda sp: sp()._extracellular()._grid_id
            if isinstance(sp(), species.ParameterOnExtracellular)
            else sp()._grid_id
        )

        # Set the local ids of the regions and species involved in the reactions
        self._ecs_species_ids = {}
        self._ecs_params_ids = {}
        for sid, s in enumerate(self._ecs_react_species):
            self._ecs_species_ids[s()._grid_id] = sid
        for sid, s in enumerate(self._ecs_react_params):
            self._ecs_params_ids[s()._grid_id] = sid

        # Setup the matrix to the ECS grid points
        for sid, s in enumerate(self._ecs_react_species + self._ecs_react_params):
            seg_idx = 0
            for sec in self._overlap:
                for seg in sec:
                    (x, y, z) = species._xyz(seg)
                    self.ecs_location_index[sid][seg_idx] = s().index_from_xyz(x, y, z)
                    seg_idx += 1
        self.ecs_location_index = self.ecs_location_index.transpose()

    def _initalize(self):
        from .species import Species, SpeciesOnRegion

        self.num_segments = numpy.sum([x.nseg for x in self._overlap])
        self.location_index = -numpy.ones(
            (self.num_regions, self.num_species + self.num_params, self.num_segments),
            ctypes.c_int,
        )
        from .species import SpeciesOnExtracellular, SpeciesOnRegion, ParameterOnRegion

        # Set the local ids of the regions and species involved in the reactions
        self._species_ids = {}
        self._params_ids = {}
        self._region_ids = {}
        self._react_species.sort(
            key=lambda sp: sp()._species()._id
            if isinstance(sp(), SpeciesOnRegion)
            else sp()._id
        )
        self._react_params.sort(
            key=lambda sp: sp()._species()._id
            if isinstance(sp(), ParameterOnRegion)
            else sp()._id
        )

        self._regions.sort(key=lambda rp: rp()._id)

        for rid, r in enumerate(self._regions):
            self._region_ids[r()._id] = rid
        for sid, s in enumerate(self._react_species):
            self._species_ids[s()._id] = sid
        for sid, s in enumerate(self._react_params):
            self._params_ids[s()._id] = sid

        # Setup the array to the state index
        for rid, r in enumerate(self._regions):
            for sid, s in enumerate(self._react_species + self._react_params):
                indices = s()._indices1d(r())
                if len(indices) > self.num_segments:
                    indices = s()._indices1d(r(), self._overlap)
                try:
                    if indices == []:
                        self.location_index[rid][sid][:] = -1
                    else:
                        self.location_index[rid][sid][:] = indices
                except ValueError:
                    raise RxDException(
                        "Rates and Reactions with species defined on different regions are not currently supported."
                    )
        self.location_index = self.location_index.transpose()

        if self._voltage_dependent:
            self._vptrs = []
            for sec in self._overlap:
                for seg in sec:
                    self._vptrs.append(seg._ref_v)

        # Setup the matrix to the ECS grid points
        if self.num_ecs_species + self.num_ecs_params > 0:
            self._ecs_initalize()
        self._initialized = True


class Extracellular:
    """Declare an extracellular region

    tortuosity = increase in path length due to obstacles, effective diffusion coefficient d/tortuosity^2. - either a single value for the whole region or a Vector giving a value for each voxel.

    Assumes reflective boundary conditions, so you'll probably want to make sure the mesh extends far beyond the neuron

    Assumes volume_fraction=1, i.e. that the extracellular space is empty. You'll probably want to lower this to
    account for other cells. Note: the section volumes are assumed to be negligible and are ignored by the simulation.

    Assumes tortuosity=1.
    """

    def __init__(
        self,
        xlo,
        ylo,
        zlo,
        xhi,
        yhi,
        zhi,
        dx,
        volume_fraction=1,
        tortuosity=None,
        permeability=None,
    ):
        from . import options

        if not options.enable.extracellular:
            raise RxDException(
                "Extracellular diffusion support is disabled. Override with rxd.options.enable.extracellular = True."
            )
        self._xlo, self._ylo, self._zlo = xlo, ylo, zlo
        if not hasattr(dx, "__len__"):
            self._dx = (dx, dx, dx)
        elif len(dx) == 3:
            self._dx = dx
        else:
            raise RxDException(
                "Extracellular region dx=%s is invalid, dx should be a number or a tuple (dx,dy,dz) for the length, width and height of the voxels"
                % repr(dx)
            )

        self._nx = int(math.ceil(float(xhi - xlo) / self._dx[0]))
        self._ny = int(math.ceil(float(yhi - ylo) / self._dx[1]))
        self._nz = int(math.ceil(float(zhi - zlo) / self._dx[2]))
        self._xhi, self._yhi, self._zhi = (
            xlo + float(self._dx[0]) * self._nx,
            ylo + float(self._dx[1]) * self._ny,
            zlo + float(self._dx[2]) * self._nz,
        )

        self._alpha, self._ecs_alpha = self._parse_volume_fraction(volume_fraction)

        if tortuosity is not None and permeability is not None:
            raise RxDException(
                "Specify either permeability or tortuosity, not both; permeability=1/tortuosity**2"
            )
        elif tortuosity is None and permeability is None:
            tortuosity = 1.0

        if permeability is None:
            self._tortuosity, self._ecs_permeability = self._parse_tortuosity(
                tortuosity
            )
        else:
            self._permeability, self._ecs_permeability = self._parse_tortuosity(
                permeability, True
            )

    def __repr__(self):
        return (
            "Extracellular(xlo=%r, ylo=%r, zlo=%r, xhi=%r, yhi=%r, zhi=%r, tortuosity=%r, volume_fraction=%r)"
            % (
                self._xlo,
                self._ylo,
                self._zlo,
                self._xhi,
                self._yhi,
                self._zhi,
                self.tortuosity,
                self.alpha,
            )
        )

    def _short_repr(self):
        return "Extracellular"

    def volume(self, index=None):
        """Returns the volume of the voxel at a given index"""
        if index is None:
            if numpy.isscalar(self.alpha):
                vol = self._nx * self._ny * self._nz * numpy.prod(self._dx) * self.alpha
            else:
                vol = numpy.sum(self.alpha) * numpy.prod(self._dx)
            return vol
        if numpy.isscalar(self.alpha):
            return numpy.prod(self._dx) * self.alpha
        return numpy.prod(self._dx) * self.alpha[index]

    @property
    def _volume_fraction_vector(self):
        if self._ecs_alpha is None:
            self._ecs_alpha = self.alpha._extracellular_instances[self]._states
        return self._ecs_alpha

    @property
    def _permeability_vector(self):
        if self._ecs_permeability is None:
            self._ecs_permeability = self.permeability._extracellular_instances[
                self
            ]._states
        return self._ecs_permeability

    def _parse_tortuosity(self, value, is_permeability=False):
        if numpy.isscalar(value):
            parsed_value = value
            ecs_permeability = value if is_permeability else value ** (-2)
        elif callable(value):
            ecs_permeability = h.Vector()
            parsed_value = numpy.ndarray((self._nx, self._ny, self._nz))
            for i in range(self._nx):
                for j in range(self._ny):
                    for k in range(self._nz):
                        parsed_value[i, j, k] = value(
                            self._xlo + i * self._dx[0],
                            self._ylo + j * self._dx[1],
                            self._zlo + k * self._dx[2],
                        )
            if is_permeability:
                ecs_permeability = h.Vector(parsed_value.flatten())
            else:
                ecs_permeability = h.Vector(parsed_value.flatten()).pow(-2)
        elif hasattr(value, "_extracellular_regions") or hasattr(
            value, "_extracellular"
        ):
            # Check Species or SpeciesOnExtracellular is defined on this region
            if (
                hasattr(value, "_extracellular_regions")
                and self not in value._extracellular_regions
            ) or (
                (
                    hasattr(value, "_extracellular")
                    and value._extracellular()
                    and value._extracellular()._region != self
                )
            ):
                raise RxDException(
                    f"permeability can be set to a State or Parameter, but it must be defined on this Extracellular region {self}"
                )
            else:
                # make sure permeability has been initialized
                if hasattr(value, "_extracellular_regions") and not hasattr(
                    value, "_extracellular_instances"
                ):
                    initializer._do_init()
                    value._finitialize()

                if is_permeability:
                    parsed_value = value
                    ecs_permeability = None
                else:
                    raise RxDException(
                        "Only permeability can be set to a State or Parameter. The tortuosity must be a; scalar, function (of x,y,z locations), or array the size of the extracellular space"
                    )
        elif isinstance(value, dict):
            if is_permeability:
                from .species import State

                params = value.copy()
                params["regions"] = [self]
                parsed_value = State(**params)
                ecs_permeability = None
            else:
                raise RxDException(
                    "Only permeability can be set to a State or Parameter. The tortuosity must be a; scalar, function (of x,y,z locations), or array the size of the extracellular space"
                )

        else:
            parsed_value = numpy.array(value)
            if parsed_value.shape != (self._nx, self._ny, self._nz):
                raise RxDException(
                    "permeability or tortuosity must be a scalar, a function of the (x,y,z) location or an array the same size as the grid: {0}x{1}x{2}".format(
                        self._nx, self._ny, self._nz
                    )
                )

            else:
                if is_permeability:
                    ecs_permeability = h.Vector(parsed_value.flatten())
                else:
                    ecs_permeability = h.Vector(parsed_value.flatten()).pow(-2)
        return parsed_value, ecs_permeability

    def _parse_volume_fraction(self, volume_fraction):

        if numpy.isscalar(volume_fraction):
            alpha = float(volume_fraction)
            alpha = alpha
            ecs_alpha = alpha
        elif isinstance(volume_fraction, dict):
            from .species import State

            params = volume_fraction.copy()
            params["regions"] = [self]
            alpha = State(**params)
            ecs_alpha = None
            warnings.warn(
                "Dynamic changes to the volume fraction does not change the concentrations of species already in the region."
            )
        elif hasattr(volume_fraction, "_extracellular_regions") or hasattr(
            volume_fraction, "_extracellular"
        ):
            # Check Species or SpeciesOnExtracellular is defined on this region
            if (
                hasattr(volume_fraction, "_extracellular_regions")
                and self not in volume_fraction._extracellular_regions
            ) or (
                (
                    hasattr(volume_fraction, "_extracellular")
                    and volume_fraction._extracellular()
                    and volume_fraction._extracellular()._region != self
                )
            ):
                raise RxDException(
                    f"volume fraction can be set to a State or Parameter, but it must be defined on this Extracellular region {self}"
                )
            else:
                # make sure volume_fraction has been initialized
                if hasattr(volume_fraction, "_extracellular_regions") and not hasattr(
                    volume_fraction, "_extracellular_instances"
                ):
                    initializer._do_init()
                    volume_fraction._finitialize()
                alpha = volume_fraction
                ecs_alpha = None
                warnings.warn(
                    "Dynamic changes to the volume fraction does not change the concentrations of species already in the region."
                )

        elif callable(volume_fraction):
            alpha = numpy.ndarray((self._nx, self._ny, self._nz))
            for i in range(self._nx):
                for j in range(self._ny):
                    for k in range(self._nz):
                        alpha[i, j, k] = volume_fraction(
                            self._xlo + i * self._dx[0],
                            self._ylo + j * self._dx[1],
                            self._zlo + k * self._dx[2],
                        )
                alpha = alpha
                ecs_alpha = h.Vector(alpha.flatten())
        else:
            alpha = numpy.array(volume_fraction)
            if alpha.shape != (self._nx, self._ny, self._nz):
                raise RxDException(
                    "free volume fraction alpha must be a scalar or an array the same size as the grid: {0}x{1}x{2}".format(
                        self._nx, self._ny, self._nz
                    )
                )

            else:
                alpha = h.Vector(alpha)
                ecs_alpha = self._alpha.as_numpy().reshape(self._nx, self._ny, self._nz)
        return alpha, ecs_alpha

    @property
    def alpha(self):
        return self._alpha

    @alpha.setter
    def alpha(self, value):
        """Set the value of the volume fraction for all species define on
           this Extracellular region. Changing the volume fraction will not
           change the concentrations already there, it will alter the
           magnitude of concentration changes due to currents.

        Args:
            value (float, array, callable or State) the new volume fraction, it
                can be a scalar for homogeneous porosity, a array the size of
                the extracellular space, a function that take (x,y,z) as an
                argument and will be evaluated on every extracellular voxel, or
                a State defined on the same extracellular region.
        """

        from .species import _update_volume_fraction

        self._alpha, self._ecs_alpha = self._parse_volume_fraction(value)
        _update_volume_fraction(self)

    @property
    def tortuosity(self):
        if hasattr(self, "_tortuosity"):
            return self._tortuosity
        return (1.0 / self._permeability) ** 0.5

    @property
    def permeability(self):
        if hasattr(self, "_tortuosity"):
            return 1.0 / self._tortuosity**2
        return self._permeability

    @tortuosity.setter
    def tortuosity(self, value):
        """Set the value of the tortuosity for all species define on this
           Extracellular region.

        Args:
            value (float, array or callable) the new tortuosity, it can be a
                scalar for homogeneous diffusion, or a array the size of the
                extracellular space or a function that take (x,y,z) as an
                argument and will be evaluated on every extracellular voxel.
        """

        from .species import _update_tortuosity

        self._tortuosity, self._ecs_permeability = self._parse_tortuosity(value)
        if hasattr(self, "_permeability"):
            delattr(self, "_permeability")

        # update tortuosity for any species defined on this ECS
        _update_tortuosity(self)

    @permeability.setter
    def permeability(self, value):
        """Set the value of the permeability for all species define on this
           Extracellular region.

        Args:
            value (float, array or callable) the new tortuosity, it can be a
                scalar for homogeneous diffusion, or a array the size of the
                extracellular space or a function that take (x,y,z) as an
                argument and will be evaluated on every extracellular voxel, or
                an rxd State or Parameter defined on the extracellular region.
        """

        from .species import _update_tortuosity

        self._permeability, self._ecs_permeability = self._parse_tortuosity(value, True)
        if hasattr(self, "_tortuosity"):
            delattr(self, "_tortuosity")

        # update tortuosity for any species defined on this ECS
        _update_tortuosity(self)


class Region(object):
    """Declare a conceptual region of the neuron.

    Examples: Cytosol, ER
    """

    def __repr__(self):
        # Note: this used to print out dimension, but that's now on a per-segment basis
        # TODO: remove the note when that is fully true
        return "Region(..., nrn_region=%r, geometry=%r, dx=%r, name=%r)" % (
            self.nrn_region,
            self._geometry,
            self.dx,
            self._name,
        )

    def __contains__(self, item):
        try:
            if item.region == self:
                return True
            else:
                return False
        except:
            raise NotImplementedError()

    def _short_repr(self):
        if self._name is not None:
            return str(self._name)
        else:
            return self.__repr__()

    def mesh_eval(self):
        from .species import _all_species

        for spref in _all_species:
            sp = spref()
            if sp is not None:
                if self in sp.regions:
                    break
        else:
            return {"instantiated": False}
        result = {"instantiated": True}
        total_num_3d_segs = sum(sec.nseg for sec in self._secs3d)
        segs_with_surface = {node.segment for node in sp.nodes if node.surface_area}
        result["3dsegswithoutsurface"] = len(segs_with_surface) - total_num_3d_segs
        return result

    def _do_init(self):
        global _region_count

        # _do_init can be called multiple times due to a change in geometry
        if hasattr(self, "_allow_setting"):
            # here are things that must only happen once
            del self._allow_setting
            self._id = _region_count
            _region_count += 1

        from . import rxd

        # parameters that were defined in old init
        # TODO: remove need for this bit
        nrn_region = self.nrn_region

        # TODO: self.dx needs to be removed... eventually that should be on a per-section basis
        #       right now, it has to be consistent but this is unenforced
        if self.dx is None:
            self.dx = 0.25
        dx = self.dx

        self._secs1d = []
        self._secs3d = []

        dims = rxd._domain_lookup
        for sec in self._secs:
            dim = dims(sec)
            if dim == 1:
                self._secs1d.append(sec)
            elif dim == 3:
                self._secs3d.append(sec)
            else:
                raise RxDException(f"unknown dimension: {dim} in section {sec}")

        # TODO: I used to not sort secs in 3D if hasattr(self._secs, 'sections'); figure out why
        self._secs = _sort_secs(self._secs)
        self._secs1d = _sort_secs(self._secs1d)
        self._secs3d = h.SectionList(self._secs3d)

        if any(self._secs3d):
            if not (hasattr(self._geometry, "volumes3d")):
                raise RxDException(
                    f'selected geometry ({self._geometry}) does not support 3d mode (no "volumes3d" attr)'
                )

            if nrn_region == "o":
                raise RxDException('3d simulations do not support nrn_region="o" yet')

            if hasattr(self, "_voxelization"):
                internal_voxels, surface_voxels, mesh_grid = (
                    self._voxelization["internal_voxels"],
                    self._voxelization["surface_voxels"],
                    self._voxelization["mesh_grid"],
                )
            else:
                internal_voxels, surface_voxels, mesh_grid = self._geometry.volumes3d(
                    self._secs3d, dx=dx
                )

            self._sa = numpy.zeros(len(surface_voxels) + len(internal_voxels))
            self._vol = numpy.ones(len(surface_voxels) + len(internal_voxels))
            self._mesh_grid = mesh_grid
            self._points = [key for key in surface_voxels.keys()] + [
                key for key in internal_voxels.keys()
            ]

            self._points = sorted(self._points)

            nodes_by_seg = {}
            surface_nodes_by_seg = {}
            # creates tuples of x, y, and z coordinates where a point is (xs[i], ys[i], zs[i])
            self._xs, self._ys, self._zs = zip(*self._points)
            maps = {
                seg: i
                for i, seg in enumerate([seg for sec in self._secs3d for seg in sec])
            }
            segs = []

            for i, p in enumerate(self._points):
                if p in surface_voxels:
                    vx = surface_voxels[p]
                    self._vol[i], self._sa[i], idx = vx[0], vx[1], maps[vx[2]]
                    segs.append(idx)

                    surface_nodes_by_seg.setdefault(idx, [])
                    surface_nodes_by_seg[idx].append(i)

                    nodes_by_seg.setdefault(idx, [])
                    nodes_by_seg[idx].append(i)
                    self._sa[i] = surface_voxels[p][1]

                else:
                    vx = internal_voxels[p]
                    self._vol[i], idx = vx[0], maps[vx[1]]
                    segs.append(idx)

                    nodes_by_seg.setdefault(idx, [])
                    nodes_by_seg[idx].append(i)

            self._surface_nodes_by_seg = surface_nodes_by_seg
            self._nodes_by_seg = nodes_by_seg
            self._secs3d_names = {
                sec.hoc_internal_name(): sec.nseg for sec in self._secs3d
            }
            self._segsidx = segs
            self._dx = self.dx

    def _segs3d(self, index=None):
        segs = [seg for sec in self._secs3d for seg in sec]
        if index:
            return [seg for i, seg in enumerate(segs) if i in index]
        return segs

    def _indices_from_sec_x(self, sec, position):
        # TODO: the assert is here because the diameter is not computed correctly
        #       unless it coincides with a 3d point, which we only know to exist at the
        #       endpoints and because the section does not proceed linearly between
        #       the endpoints (in general)... which affects the computation of the
        #       normal vector as well
        assert position in (0, 1)
        # NOTE: some care is necessary in constructing normal vector... must be
        #       based on end frusta, not on vector between end points
        dx = self.dx
        if position == 0:
            x = sec.x3d(0)
            y = sec.y3d(0)
            z = sec.z3d(0)
            nx = sec.x3d(1) - x
            ny = sec.y3d(1) - y
            nz = sec.z3d(1) - z
        elif position == 1:
            n = sec.n3d()
            x = sec.x3d(n - 1)
            y = sec.y3d(n - 1)
            z = sec.z3d(n - 1)
            nx = x - sec.x3d(n - 2)
            ny = y - sec.y3d(n - 2)
            nz = z - sec.z3d(n - 2)
            scale = dx / (nx**2 + ny**2 + nz**2) ** 0.5
            x -= nx * scale
            y -= ny * scale
            z -= nz * scale

        else:
            raise RxDException("should never get here")
        if nx == 0 and ny == 0 and nz == 0:
            raise RxDException(
                "The 3D/1D join on section %r cannot be calculated from identical 3d points %g, %g, %g"
                % (sec, x, y, z)
            )
        # dn = (nx**2 + ny**2 + nz**2)**0.5
        # nx, ny, nz = nx/dn, ny/dn, nz/dn
        # x, y, z = x * x1 + (1 - x) * x0, x * y1 + (1 - x) * y0, x * z1 + (1 - x) * z1
        r = sec(position).diam * 0.5 + self.dx * 3**0.5
        plane_of_disc = geometry3d.graphicsPrimitives.Plane(x, y, z, nx, ny, nz)

        xs = numpy.arange(
            self._mesh_grid["xlo"] - self._mesh_grid["dx"],
            self._mesh_grid["xhi"] + self._mesh_grid["dx"],
            self._mesh_grid["dx"],
        )
        ys = numpy.arange(
            self._mesh_grid["ylo"] - self._mesh_grid["dy"],
            self._mesh_grid["yhi"] + self._mesh_grid["dy"],
            self._mesh_grid["dy"],
        )
        zs = numpy.arange(
            self._mesh_grid["zlo"] - self._mesh_grid["dz"],
            self._mesh_grid["zhi"] + self._mesh_grid["dz"],
            self._mesh_grid["dz"],
        )
        xlo = self._mesh_grid["xlo"] - self._mesh_grid["dx"]
        ylo = self._mesh_grid["ylo"] - self._mesh_grid["dy"]
        zlo = self._mesh_grid["zlo"] - self._mesh_grid["dz"]
        # locate the indices of the cube containing the sphere containing the disc
        # TODO: write this more efficiently
        i_indices = [i for i, a in enumerate(xs) if abs(a - x) <= r]
        j_indices = [i for i, a in enumerate(ys) if abs(a - y) <= r]
        k_indices = [i for i, a in enumerate(zs) if abs(a - z) <= r]
        sphere_indices = [
            (i, j, k)
            for i, j, k in itertools.product(i_indices, j_indices, k_indices)
            if (xs[i] - x) ** 2 + (ys[j] - y) ** 2 + (zs[k] - z) ** 2 <= r**2
        ]
        dx = self.dx
        disc_indices = []
        for i, j, k in sphere_indices:
            #            a, b, c = xs[i], ys[j], zs[k]
            # TODO: no need to compute all; can stop when some True and some False
            #            on_side1 = [plane_of_disc.distance(x, y, z) >= 0 for x, y, z in itertools.product([a - dx2, a + dx2], [b - dx2, b + dx2], [c - dx2, c + dx2])]
            # NOTE: the expression is structured this way to make sure it tests the exact same corner coordinates for corners shared by multiple voxels and that there are no round-off issues (an earlier attempt had round-off issues that resulted in double-thick discs when the frustum ended exactly on a grid plane)
            on_side1 = [
                plane_of_disc.distance(x, y, z) > 0
                for x, y, z in itertools.product(
                    [xlo + (i - 0.5) * dx, xlo + (i + 0.5) * dx],
                    [ylo + (j - 0.5) * dx, ylo + (j + 0.5) * dx],
                    [zlo + (k - 0.5) * dx, zlo + (k + 0.5) * dx],
                )
            ]
            # need both sides to have at least one corner
            if any(on_side1) and not all(on_side1):
                # if we're here, then we've found a point on the disc.
                disc_indices.append((i - 1, j - 1, k - 1))
        return disc_indices

    def __init__(
        self,
        secs=None,
        nrn_region=None,
        geometry=None,
        dimension=None,
        dx=None,
        name=None,
    ):
        """
        In NEURON 7.4+, secs is optional at initial region declaration, but it
        must be specified before the reaction-diffusion model is instantiated.

        .. note:: dimension and dx will be deprecated in a future version
        """
        self._allow_setting = True
        if hasattr(secs, "__len__"):
            self._secs = secs
        else:
            self._secs = [secs]
        if not secs:
            warnings.warn(
                "Warning: No sections. Region 'secs' should be a list of NEURON sections."
            )
        from nrn import Section

        for sec in self._secs:
            if not isinstance(sec, Section):
                raise RxDException(
                    f"Error: Region 'secs' must be a list of NEURON sections, {sec} is not a valid NEURON section."
                )
        self._secs = h.SectionList(self._secs)
        self.geometry = geometry
        self._name = name
        self.nrn_region = nrn_region

        if dimension is not None:
            warnings.warn(
                "dimension argument was a development feature only; use set_solve_type instead... the current version sets all the sections to your requested dimension, but this will override any previous settings"
            )
            import neuron

            neuron.rxd.set_solve_type(secs, dimension=dimension)
        if dx is not None:
            try:
                dx = float(dx)
            except:
                dx = -1
            if dx <= 0:
                raise RxDException("dx must be a positive real number or None")
        self.dx = dx
        _all_regions.append(weakref.ref(self))

        # initialize self if the rest of rxd is already initialized
        if initializer.is_initialized():
            self._do_init()

    @property
    def nrn_region(self):
        """Get or set the classic NEURON region associated with this object.

        There are three possible values:
            * `'i'` -- just inside the plasma membrane
            * `'o'` -- just outside the plasma membrane
            * `None` -- none of the above

        .. note:: Setting only supported in NEURON 7.4+, and then only before the reaction-diffusion model is instantiated.
        """
        return self._nrn_region

    @nrn_region.setter
    def nrn_region(self, value):
        if hasattr(self, "_allow_setting"):
            if value not in (None, "i", "o"):
                raise RxDException('nrn_region must be one of: None, "i", "o"')
            else:
                self._nrn_region = value
            if (
                value == "i"
                and isinstance(self.geometry, FractionalVolume)
                and self.geometry._surface_fraction == 0
            ):
                warnings.warn(
                    f'{self.name} is in the "i" region with a surface_fraction of 0 and will not have currents added'
                )
        else:
            raise RxDException("Cannot set nrn_region now; model already instantiated")

    @property
    def geometry(self):
        """Get or set the geometry associated with this region.

        Setting the geometry to `None` will cause it to default to `rxd.geometry.inside`.

        .. note:: New in NEURON 7.4+. Setting allowed only before the reaction-diffusion model is instantiated.
        """
        return self._geometry

    @geometry.setter
    def geometry(self, value):
        if hasattr(self, "_allow_setting"):
            if value is None:
                value = geo.inside
            self._geometry = value
        else:
            raise RxDException("Cannot set geometry now; model already instantiated")

    @property
    def name(self):
        """Get or set the Region's name.

        .. note:: New in NEURON 7.4+.
        """
        return self._name

    @name.setter
    def name(self, value):
        self._name = value

    @property
    def _semi_compile(self):
        return f"r{self._id}"

    @property
    def secs(self):
        """Get or set the sections associated with this region.

        The sections may be expressed as a NEURON SectionList or as any Python
        iterable of sections.

        Note: The return value is a copy of the internal section list; modifying
              it will not change the Region.

        .. note:: Setting is new in NEURON 7.4+ and allowed only before the reaction-diffusion model is instantiated.
        """
        if hasattr(self._secs, "__len__"):
            return list(self._secs)
        else:
            return [self._secs]

    @secs.setter
    def secs(self, value):
        if hasattr(self, "_allow_setting"):
            if hasattr(value, "__len__"):
                secs = value
            else:
                secs = [value]

            from nrn import Section

            for sec in secs:
                if not isinstance(sec, Section):
                    raise RxDException(
                        f"Error: Region 'secs' must be a list of NEURON sections, {sec} is not a valid NEURON section."
                    )
            self._secs = h.SectionList(secs)
        else:
            raise RxDException("Cannot set secs now; model already instantiated")

    def volume(self, index=None):
        """Returns the volume of the voxel at a given index"""
        initializer._do_init()
        if index is None:
            vol = 0
            if hasattr(self, "_vol") and any(self._secs3d):
                vol += numpy.sum(self._vol)
            if hasattr(self, "_geometry") and any(self._secs1d):
                vol += numpy.sum(
                    [self._geometry.volumes1d(sec) for sec in self._secs1d]
                )
            return vol
        return self._vol[index]

from .rxdmath import _Arithmeticed
import weakref
from .section1d import Section1D
from neuron import h, nrn, nrn_dll_sym
from . import node, nodelist, rxdmath, region
import numpy
import warnings
import itertools
from . import options
from .rxdException import RxDException
from . import initializer
from collections.abc import Callable


import ctypes
import re

# Now set in rxd.py
# set_nonvint_block = neuron.nrn_dll_sym('set_nonvint_block')

# Update the structure_change_cnt & diam_change_cnt if the shape has changed
_nrn_shape_update = nrn_dll_sym("nrn_shape_update_always")

fptr_prototype = ctypes.CFUNCTYPE(None)

set_setup = nrn_dll_sym("set_setup")
set_setup.argtypes = [fptr_prototype]
set_initialize = nrn_dll_sym("set_initialize")
set_initialize.argtypes = [fptr_prototype]

# setup_solver = nrn.setup_solver
# setup_solver.argtypes = [ctypes.py_object, ctypes.py_object, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double]

ECS_insert = nrn_dll_sym("ECS_insert")
ECS_insert.argtypes = [
    ctypes.c_int,
    ctypes.py_object,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.py_object,
    ctypes.py_object,
    ctypes.c_int,
    ctypes.c_double,
    ctypes.c_double,
]

ECS_insert.restype = ctypes.c_int

ICS_insert = nrn_dll_sym("ICS_insert")
ICS_insert_inhom = nrn_dll_sym("ICS_insert_inhom")

ICS_insert_inhom.argtypes = ICS_insert.argtypes = [
    ctypes.c_int,
    ctypes.py_object,
    ctypes.c_long,
    numpy.ctypeslib.ndpointer(dtype=int),
    numpy.ctypeslib.ndpointer(dtype=int),
    ctypes.c_long,
    numpy.ctypeslib.ndpointer(dtype=int),
    ctypes.c_long,
    numpy.ctypeslib.ndpointer(dtype=int),
    ctypes.c_long,
    numpy.ctypeslib.ndpointer(dtype=float),
    ctypes.c_double,
    ctypes.c_bool,
    ctypes.c_double,
    numpy.ctypeslib.ndpointer(dtype=float),
]

ICS_insert.restype = ctypes.c_int
ICS_insert_inhom.restype = ctypes.c_int

# function to change extracellular diffusion
set_diffusion = node.set_diffusion

species_atolscale = nrn_dll_sym("species_atolscale")
species_atolscale.argtypes = [
    ctypes.c_int,
    ctypes.c_double,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_int),
]

remove_species_atolscale = nrn_dll_sym("remove_species_atolscale")
remove_species_atolscale.argtypes = [ctypes.c_int]

_set_grid_concentrations = nrn_dll_sym("set_grid_concentrations")
_set_grid_concentrations.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    ctypes.py_object,
    ctypes.py_object,
]

_ics_set_grid_concentrations = nrn_dll_sym("ics_set_grid_concentrations")
_ics_set_grid_concentrations.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    numpy.ctypeslib.ndpointer(dtype=numpy.int64),
    numpy.ctypeslib.ndpointer(dtype=numpy.int64),
    ctypes.py_object,
]

_set_grid_currents = nrn_dll_sym("set_grid_currents")
_set_grid_currents.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    ctypes.py_object,
    ctypes.py_object,
    ctypes.py_object,
]

_ics_set_grid_currents = nrn_dll_sym("ics_set_grid_currents")
_ics_set_grid_currents.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    ctypes.py_object,
    numpy.ctypeslib.ndpointer(dtype=float),
]


_delete_by_id = nrn_dll_sym("delete_by_id")
_delete_by_id.argtypes = [ctypes.c_int]

# function to change extracellular tortuosity
_set_tortuosity = nrn_dll_sym("set_tortuosity")
_set_tortuosity.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.py_object]


# The difference here is that defined species only exists after rxd initialization
_all_species = []
_defined_species = {}
_all_defined_species = []
_defined_species_by_gid = []


def _get_all_species():
    return _all_defined_species


_extracellular_diffusion_objects = weakref.WeakKeyDictionary()
_intracellular_diffusion_objects = weakref.WeakKeyDictionary()

_species_count = 0

_has_1d = False
_has_3d = False


def _update_tortuosity(region):
    """Update tortuosity for all species on region"""

    for s, r in _extracellular_diffusion_objects.items():
        if (
            r == region
            and hasattr(s, "_id")
            and not hasattr(s, "_deleted")
            and not s._diffusion_characteristic
        ):
            _set_tortuosity(s._id, region._permeability_vector)


def _update_volume_fraction(region):
    """Update volume fractions for all species on region"""

    for s, r in _extracellular_diffusion_objects.items():
        if (
            r == region
            and hasattr(s, "_id")
            and not hasattr(s, "_deleted")
            and not s._diffusion_characteristic
        ):
            _set_volume_fraction(s._id, region._volume_faction_vector)


def _1d_submatrix_n():
    if not _has_1d:
        return 0
    else:
        return len(node._states)
    """elif not _has_3d:
        return len(node._states)
    else:
        return numpy.min([sp()._indices3d() for sp in list(_get_all_species()) if sp() is not None])"""


_ontology_id = re.compile(
    "^\[[a-zA-Z][a-zA-Z0-9_]*:[a-zA-Z0-9_]*\]|[a-zA-Z][a-zA-Z0-9_]*:[a-zA-Z0-9_]*$"
)


class _SpeciesMathable(object):
    # support arithmeticing
    def __neg__(self):
        return -1 * _Arithmeticed(self)

    def __Pos__(self):
        return _Arithmeticed(self)

    def __pow__(self, other):
        return _Arithmeticed(self) ** other

    def __add__(self, other):
        return _Arithmeticed(self) + other

    def __sub__(self, other):
        return _Arithmeticed(self) - other

    def __mul__(self, other):
        return _Arithmeticed(self) * other

    def __div__(self, other):
        return _Arithmeticed(self) / other

    def __rdiv__(self, other):
        return _Arithmeticed(other) / self

    def __radd__(self, other):
        return _Arithmeticed(other) + self

    def __rmul__(self, other):
        return _Arithmeticed(self) * other

    def __truediv__(self, other):
        return _Arithmeticed(self) / other

    def __rtruediv__(self, other):
        return _Arithmeticed(other) / self

    def __rfloordiv__(self, other):
        return _Arithmeticed(other) / self

    def __rsub__(self, other):
        return _Arithmeticed(other) - self

    def __abs__(self):
        return abs(_Arithmeticed(self))

    def _evaluate(self, location):
        return _Arithmeticed(self)._evaluate(location)

    def __ne__(self, other):
        other = rxdmath._ensure_arithmeticed(other)
        self2 = rxdmath._ensure_arithmeticed(self)
        rxdmath._validate_reaction_terms(self2, other)
        return rxdmath._Reaction(self2, other, "<>")

    def __gt__(self, other):
        other = rxdmath._ensure_arithmeticed(other)
        self2 = rxdmath._ensure_arithmeticed(self)
        rxdmath._validate_reaction_terms(self2, other)
        return rxdmath._Reaction(self2, other, ">")

    def __lt__(self, other):
        other = rxdmath._ensure_arithmeticed(other)
        self2 = rxdmath._ensure_arithmeticed(self)
        rxdmath._validate_reaction_terms(self2, other)
        return rxdmath._Reaction(self2, other, "<")

    def _semi_compile(self, reg, instruction):
        from . import region

        # region is Extracellular

        # if the species is only define on one extracellular region then use that
        if len(self._regions) == 0 and len(self._extracellular_regions) == 1:
            reg = self._extracellular_regions[0]

        # region is Extracellular
        if isinstance(reg, region.Extracellular) or len(self._regions) == 0:
            ecs_instance = self._extracellular_instances[reg]
            return ecs_instance._semi_compile(reg, instruction)
        # region is 3d intracellular
        if isinstance(reg, region.Region) and instruction == "do_3d":
            ics_instance = self._intracellular_instances[reg]
            return ics_instance._semi_compile(reg, instruction)
        if isinstance(reg, region.Region) and instruction == "do_1d":
            if reg in self._regions:
                return "species[%d][%d]" % (self._id, reg._id)
            else:
                raise RxDException(
                    "Species %r is not defined on region %r." % (self, reg)
                )
        raise RxDException("_semi_compile received inconsistent state")

    def _involved_species(self, the_dict):
        the_dict[self._semi_compile] = weakref.ref(self)

    @property
    def d(self):
        return self._d

    @d.setter
    def d(self, value):
        from . import rxd

        self._d = value
        if hasattr(self, "_extracellular_instances"):
            for ecs in self._extracellular_instances.values():
                ecs.d = value
        if hasattr(self, "_intracellular_instances"):
            for ics in self._intracellular_instances.values():
                ics.d = value
        if initializer.is_initialized() and hasattr(self, "_region_indices"):
            _volumes, _surface_area, _diffs = node._get_data()
            if _diffs.size > 0:
                if hasattr(value, "__len__"):
                    raise RxDException(
                        "Intracellular 1D can only have one diffusion coefficient. To set 3D intracellular or extracellular diffusion, access the species on the regions; `species[region].d = (Dx, Dy, Dz)`"
                    )
                _diffs[self._indices1d()] = value
                rxd._setup_matrices()


class SpeciesOnExtracellular(_SpeciesMathable):
    def __init__(self, species, extracellular):
        """The restriction of a Species to a Region."""
        self._species = weakref.ref(species)
        self._extracellular = weakref.ref(extracellular)
        self._id = _species_count

    def __repr__(self):
        return "%r[%r]" % (self._species(), self._extracellular()._region)

    def _short_repr(self):
        return "%s[%s]" % (
            self._species()._short_repr(),
            self._extracellular()._region._short_repr(),
        )

    def __str__(self):
        return "%s[%s]" % (
            self._species()._short_repr(),
            self._extracellular()._region._short_repr(),
        )

    @property
    def concentration(self):
        """An iterable of the current concentrations."""
        return self.nodes.concentration

    @concentration.setter
    def concentration(self, value):
        """Sets all Node objects in the restriction to the specified concentration value.

        This is equivalent to s.nodes.concentration = value"""
        self.nodes.concentration = value

    @property
    def states3d(self):
        return self._extracellular().states

    def extent(self, axes):
        """valid options for axes: xyz, xy, xz, yz, x, y, z"""
        e = self._extracellular()
        if axes == "xyz":
            return [e._xlo, e._xhi, e._ylo, e._yhi, e._zlo, e._zhi]
        elif axes == "xy":
            return [e._xlo, e._xhi, e._ylo, e._yhi]
        elif axes == "xz":
            return [e._xlo, e._xhi, e._zlo, e._zhi]
        elif axes == "yz":
            return [e._ylo, e._yhi, e._zlo, e._zhi]
        elif axes == "x":
            return [e._xlo, e._xhi]
        elif axes == "y":
            return [e._ylo, e._yhi]
        elif axes == "z":
            return [e._zlo, e._zhi]
        else:
            raise RxDException(
                "unknown axes argument; valid options: xyz, xy, xz, yz, x, y and z"
            )

    def node_by_location(self, x, y, z):
        e = self._extracellular()._region
        if e._xlo <= x <= e._xhi and e._ylo <= y <= e._yhi and e._zlo <= z <= e._zhi:
            i = int((x - e._xlo) / e._dx[0])
            j = int((y - e._ylo) / e._dx[1])
            k = int((z - e._zlo) / e._dx[2])
            return self.node_by_ijk(i, j, k)
        raise RxDException(
            "Location (%1.2f %1.2f, %1.2f) is not in extracellular region %r",
            x,
            y,
            z,
            self._extracellular()._region,
        )

    def alpha_by_location(self, locs):
        """Return a single alpha value for a homogeneous volume fraction of a list of alpha values for an inhomogeneous volume fraction at grid locations given in a list (locs)."""
        e = self._extracellular()._region
        if numpy.isscalar(e.alpha):
            return e.alpha
        alphas = []
        for loc in locs:
            i = int((loc[0] - e._xlo) / e._dx[0])
            j = int((loc[1] - e._ylo) / e._dx[1])
            k = int((loc[2] - e._zlo) / e._dx[2])
            alphas.append(e.alpha[i, j, k])
        return numpy.array(alphas)

    def node_by_ijk(self, i, j, k):
        index = 0
        s = self._extracellular()
        for ecs in self._species()._extracellular_instances.values():
            if ecs == s:
                e = s._region
                index += (i * e._ny + j) * e._nz + k
            else:
                e = ecs._region
                index += e._nz * e._ny * e._nz
        return self._species()._extracellular_nodes[index]

    @property
    def nodes(self):
        """A NodeList of the Node objects containing concentration data for the given Species and extracellular region.

        The code

            node_list = ca[ecs].nodes

        is more efficient than the otherwise equivalent

            node_list = ca.nodes(ecs)

        because the former only creates the Node objects belonging to the restriction ca[cyt] whereas the second option
        constructs all Node objects belonging to the Species ca and then culls the list to only include those also
        belonging to the Region cyt.
        """
        initializer._do_init()
        return nodelist.NodeList(
            [
                nd
                for nd in self._species()._extracellular_nodes
                if nd.region == self._extracellular()._region
            ]
        )

    def _semi_compile(self, reg, instruction):
        # This will always be an ecs_instance
        # reg = self._extracellular()._region
        # ecs_instance = self._species()._extracellular_instances[reg]
        # return ecs_instance._semi_compile(reg)
        return self._extracellular()._semi_compile(reg, instruction)

    @property
    def d(self):
        return self._extracellular()._d

    @d.setter
    def d(self, value):
        self._extracellular().d = value

    def defined_on_region(self, r):
        return r == self._extracellular()


class SpeciesOnRegion(_SpeciesMathable):
    def __init__(self, species, region):
        """The restriction of a Species to a Region."""
        self._species = weakref.ref(species)
        self._region = weakref.ref(region)

    def __eq__(self, other):
        """test for equality.

        Two SpeciesOnRegion objects are equal if they refer to the same species
        on the same region and both the species and the region still exist.
        """
        return (
            (self._species() == other._species())
            and (self._region() == other._region())
            and (self._species() is not None)
            and (self._region() is not None)
        )

    def __hash__(self):
        # TODO: replace this to reduce collision risk; how much of a problme is that?
        return 1000 * (hash(self._species()) % 1000) + (hash(self._region()) % 1000)

    def __repr__(self):
        return "%r[%r]" % (self._species(), self._region())

    def _short_repr(self):
        return "%s[%s]" % (self._species()._short_repr(), self._region()._short_repr())

    def __str__(self):
        return "%s[%s]" % (self._species()._short_repr(), self._region()._short_repr())

    def indices(self, r=None, secs=None):
        """If no Region is specified or if r is the Region specified in the constructor,
        returns a list of the indices of state variables corresponding
        to the Species when restricted to the Region defined in the constructor.

        If r is a different Region, then returns the empty list.
        If secs is a set return all indices on the regions for those sections.
        """
        # if r is not None and r != self._region():
        #    raise RxDException('attempt to access indices on the wrong region')
        # TODO: add a mechanism to catch if not right region (but beware of reactions crossing regions)
        if self._species() is None or self._region() is None:
            return []
        else:
            return self._species().indices(self._region(), secs)

    def __getitem__(self, r):
        """Return a reference to those members of this species lying on the specific region @varregion.
        The resulting object is a SpeciesOnRegion.
        This is useful for defining reaction schemes for MultiCompartmentReaction.

        This is provided to allow use of SpeciesOnRegion where Species would normally go.

        If the regions match, self is returned; otherwise an empty SpeciesOnRegion.
        """
        if r == self._region():
            return self
        raise RxDException("no such region")

    @property
    def states(self):
        """A vector of all the states corresponding to this species"""
        # TODO This should be restricted to the Region
        return self._species().states

    @property
    def states3D(self):
        """A vector of all the 3D states corresponding to this species"""
        # TODO This should be restricted to the Region
        return self._species()._intracellular_instances[self._region()].states

    @property
    def nodes(self):
        """A NodeList of the Node objects containing concentration data for the given Species and Region.

        The code

            node_list = ca[cyt].nodes

        is more efficient than the otherwise equivalent

            node_list = ca.nodes(cyt)

        because the former only creates the Node objects belonging to the restriction ca[cyt] whereas the second option
        constructs all Node objects belonging to the Species ca and then culls the list to only include those also
        belonging to the Region cyt.
        """
        from . import rxd

        _nrn_shape_update()
        if initializer.is_initialized():
            rxd._update_node_data()
        else:
            initializer._do_init()
        my_nodes = [
            s.nodes for s in self._species()._secs if s._region == self._region()
        ]
        try:
            my_nodes.append(self.instance3d._nodes)
        except:
            pass
        return nodelist.NodeList(itertools.chain.from_iterable(my_nodes))

    @property
    def concentration(self):
        """An iterable of the current concentrations."""
        return self.nodes.concentration

    @concentration.setter
    def concentration(self, value):
        """Sets all Node objects in the restriction to the specified concentration value.

        This is equivalent to s.nodes.concentration = value"""
        self.nodes.concentration = value

    def _semi_compile(self, reg, instruction):
        # Never an _ExtracellularSpecies since we are in SpeciesOnRegion
        reg = self._region()
        if instruction == "do_3d":
            ics_instance = self._species()._intracellular_instances[reg]
            return ics_instance._semi_compile(reg, instruction)
        elif any(reg._secs1d):
            return "species[%d][%d]" % (self._id, self._region()._id)
        else:
            raise RxDException("There are no 1D sections defined on {}".format(reg))

    def defined_on_region(self, r):
        return r == self._region()

    @property
    def instance3d(self):
        sp = self._species()
        r = self._region()
        if r in sp._intracellular_instances:
            return sp._intracellular_instances[r]
        else:
            raise RxDException("There are no 3D species defined on {}".format(r))

    @property
    def _id(self):
        return self._species()._id


# 3d matrix stuff
def _setup_matrices_process_neighbors(
    pt1, pt2, indices, euler_matrix, index, diffs, vol, areal, arear, dx
):
    # TODO: validate this before release! is ignoring reflective boundaries the right thing to do?
    #       (make sure that any changes here also work with boundaries that aren't really reflective, but have a 1d section attached)
    d = diffs[index]
    if pt1 in indices:
        ileft = indices[pt1]
        dleft = (d + diffs[ileft]) * 0.5
        left = dleft * areal / (vol * dx)
        euler_matrix[index, ileft] += left
        euler_matrix[index, index] -= left
    if pt2 in indices:
        iright = indices[pt2]
        dright = (d + diffs[iright]) * 0.5
        right = dright * arear / (vol * dx)
        euler_matrix[index, iright] += right
        euler_matrix[index, index] -= right


def _xyz(seg):
    """Return the (x, y, z) coordinate of the center of the segment."""
    # TODO: this is very inefficient, especially since we're calling this for each segment not for each section; fix
    sec = seg.sec
    n3d = sec.n3d()
    x3d = [sec.x3d(i) for i in range(n3d)]
    y3d = [sec.y3d(i) for i in range(n3d)]
    z3d = [sec.z3d(i) for i in range(n3d)]
    arc3d = [sec.arc3d(i) for i in range(n3d)]
    return (
        numpy.interp([seg.x * sec.L], arc3d, x3d)[0],
        numpy.interp([seg.x * sec.L], arc3d, y3d)[0],
        numpy.interp([seg.x * sec.L], arc3d, z3d)[0],
    )


class _IntracellularSpecies(_SpeciesMathable):
    def __init__(
        self,
        region=None,
        d=0,
        name=None,
        charge=0,
        initial=0,
        nodes=0,
        is_diffusable=True,
        atolscale=1.0,
    ):
        """
        region = Intracellular object
        name = string of the name of the NEURON species (e.g. ca)
        d = diffusion constant
        charge = charge of the molecule (used for converting currents to concentration changes)
        initial = initial concentration
        NOTE: For now, only define this AFTER the morphology is instantiated. In the future, this requirement can be relaxed.
        TODO: remove this limitation
        """
        _intracellular_diffusion_objects[self] = None
        # ensure 3D points exist
        h.define_shape()
        self._region = region
        self._species = name
        self._name = name
        self._charge = charge
        self._dx = region.dx
        self._atolscale = atolscale
        self._nodes_length = len(self._region._xs)
        self._states = h.Vector(self._nodes_length)
        self.neighbors = self.create_neighbors_array(nodes, self._nodes_length).reshape(
            3 * self._nodes_length
        )
        self._initial = initial
        self.states = self._states.as_numpy()
        self._nodes = nodes

        dc, dgrid = self._parse_diffusion(d)
        if dc is not None:
            self._d = dc
        else:
            self._dgrid = dgrid
        self._x_line_defs = self.line_defs(self._nodes, "x", self._nodes_length)
        self._y_line_defs = self.line_defs(self._nodes, "y", self._nodes_length)
        self._z_line_defs = self.line_defs(self._nodes, "z", self._nodes_length)

        self._ordered_x_nodes = self.ordered_nodes(
            self._x_line_defs, "x", self.neighbors
        )
        self._ordered_y_nodes = self.ordered_nodes(
            self._y_line_defs, "y", self.neighbors
        )
        self._ordered_z_nodes = self.ordered_nodes(
            self._z_line_defs, "z", self.neighbors
        )

        self._alphas = self.create_alphas()

        if hasattr(self, "_dgrid"):
            self._grid_id = ICS_insert_inhom(
                0,
                self._states._ref_x[0],
                len(self.states),
                self.neighbors,
                self._x_line_defs,
                len(self._x_line_defs),
                self._y_line_defs,
                len(self._y_line_defs),
                self._z_line_defs,
                len(self._z_line_defs),
                self._dgrid,
                self._dx,
                is_diffusable,
                self._atolscale,
                self._alphas,
            )
        else:
            self._grid_id = ICS_insert(
                0,
                self._states._ref_x[0],
                len(self.states),
                self.neighbors,
                self._x_line_defs,
                len(self._x_line_defs),
                self._y_line_defs,
                len(self._y_line_defs),
                self._z_line_defs,
                len(self._z_line_defs),
                self._d,
                self._dx,
                is_diffusable,
                self._atolscale,
                self._alphas,
            )

        self._update_pointers()
        _defined_species_by_gid.append(self)

    def _isalive(self):
        if self not in _intracellular_diffusion_objects:
            raise RxDException("The intracellular species has been deleted.")

    def _delete(self):
        global _intracellular_diffusion_objects, _defined_species_by_gid
        if self in _defined_species_by_gid:
            _defined_species_by_gid.remove(self)
        if _intracellular_diffusion_objects:
            if self in _intracellular_diffusion_objects:
                del _intracellular_diffusion_objects[self]
                # remove the grid id
                for sp in _intracellular_diffusion_objects:
                    if sp._grid_id > self._grid_id:
                        sp._grid_id -= 1
                _delete_by_id(self._grid_id)
                # remove any node.include_flux for the extracellular species.
                from . import node

                newflux = {
                    "index": [],
                    "type": [],
                    "source": [],
                    "scale": [],
                    "region": [],
                }
                for idx, t, src, sc, rptr in zip(
                    node._node_fluxes["index"],
                    node._node_fluxes["type"],
                    node._node_fluxes["source"],
                    node._node_fluxes["scale"],
                    node._node_fluxes["region"],
                ):
                    if t != self._grid_id:
                        if t > self._grid_id:
                            newflux["type"].append(t)
                        else:
                            newflux["type"].append(t - 1)
                        newflux["index"].append(idx)
                        newflux["type"].append(t)
                        newflux["source"].append(src)
                        newflux["scale"].append(sc)
                        newflux["region"].append(region)
                node._has_node_fluxes = newflux != node._node_fluxes
                node._node_fluxes = newflux

                # if this is the only intracellular species using
                # node._point_indices, then remove the corresponding region.
                for sp in _intracellular_diffusion_objects:
                    if sp._region == self._region:
                        break
                else:
                    if self._region in node._point_indices:
                        del node._point_indices[self._region]

                nrn_dll_sym("structure_change_cnt", ctypes.c_int).value += 1

    # Line Definitions for each direction
    def line_defs(self, nodes, direction, nodes_length):
        self._isalive()
        line_defs = []

        indices = set(range(0, nodes_length))

        pos_neg_from_dir = {"x": (0, 1), "y": (2, 3), "z": (4, 5)}
        pos, neg = pos_neg_from_dir[direction]
        while indices:
            line_start = indices.pop()
            line_length = 1
            for my_dir in [pos, neg]:
                node = nodes[line_start]
                while (node.neighbors[my_dir] is not None) and indices:
                    to_remove = node._neighbors[my_dir]
                    node = nodes[to_remove]
                    indices.remove(to_remove)
                    line_length += 1

            line_start = node._index
            line_defs.append([line_start, line_length])

        # sort list for parallelization
        line_defs.sort(key=lambda x: x[1], reverse=True)
        line_defs = numpy.asarray(line_defs, dtype=int)
        line_defs = line_defs.reshape(2 * len(line_defs))
        return line_defs

    def ordered_nodes(self, p_line_defs, direction, neighbors):
        self._isalive()
        offset_from_dir = {"x": 0, "y": 1, "z": 2}
        offset = offset_from_dir[direction]
        ordered_nodes = []
        for i in range(0, len(p_line_defs) - 1, 2):
            current_node = p_line_defs[i]
            line_length = p_line_defs[i + 1]
            ordered_nodes.append(current_node)
            for j in range(1, line_length):
                current_node = neighbors[current_node * 3 + offset]
                ordered_nodes.append(current_node)
        return numpy.asarray(ordered_nodes)

    def create_neighbors_array(self, nodes, nodes_length):
        self._isalive()
        my_array = numpy.zeros((nodes_length, 3), dtype=int)
        for n in nodes:
            for i, ele in enumerate(n.neighbors[::2]):
                my_array[n._index, i] = ele if ele is not None else -1
        return my_array

    def create_alphas(self):
        self._isalive()
        alphas = [vol / self._dx**3 for vol in self._region._vol]
        return numpy.asarray(alphas, dtype=float)

    def _import_concentration(self):
        self._isalive()
        ion = "_ref_" + self._name + self._region._nrn_region
        for seg, nodes in zip(
            r._segs3d(self._region._nodes_by_seg.keys()),
            self._region._nodes_by_seg.values(),
        ):
            segptr = getattr(seg, ion)
            value = segptr[0]
            for node in nodes:
                self._nodes[node].concentration = value

    def _finitialize(self):
        # Updated - now it will initialize using Node3D
        # TODO: support more complicated initializations than just constants
        self._isalive()
        if self._initial is None:
            if self._region._nrn_region:
                self._import_concentration()
            else:
                self.states[:] = 0

    def _update_pointers(self):
        self._isalive()
        if options.concentration_nodes_3d == "surface":
            self._seg_to_surface_nodes = self._region._surface_nodes_by_seg
        elif options.concentration_nodes_3d == "all":
            self._seg_to_surface_nodes = self._region._nodes_by_seg
        else:
            raise RxDException(
                "options.concentration_nodes_3d must be 'surface' or 'all'"
            )
        grid_list_start = 0
        if self._nodes and self._name:
            nrn_region = self._region._nrn_region
            if nrn_region:
                ion_conc = "_ref_" + self._name + nrn_region
                segs = self._region._segs3d()
                self._concentration_neuron_pointers = [
                    getattr(segs[segidx], ion_conc)
                    for segidx in self._seg_to_surface_nodes.keys()
                ]
                self._surface_nodes_per_seg = [
                    nodes for nodes in self._seg_to_surface_nodes.values()
                ]
                self._surface_nodes_per_seg_start_indices = [
                    len(nodes) for nodes in self._surface_nodes_per_seg
                ]

                # cast to numpy arrays to avoid calling PyList_GET_ITEM on every element
                self._surface_nodes_per_seg = list(
                    itertools.chain.from_iterable(self._surface_nodes_per_seg)
                )
                self._surface_nodes_per_seg = numpy.asarray(
                    self._surface_nodes_per_seg, dtype=numpy.int64
                )

                self._surface_nodes_per_seg_start_indices = numpy.cumsum(
                    [0] + self._surface_nodes_per_seg_start_indices, dtype=numpy.int64
                )

                _ics_set_grid_concentrations(
                    grid_list_start,
                    self._grid_id,
                    self._surface_nodes_per_seg,
                    self._surface_nodes_per_seg_start_indices,
                    self._concentration_neuron_pointers,
                )

                if self._name is not None and self._charge != 0:
                    nrn_region_sign = {"i": -1, "o": 1}
                    sign = nrn_region_sign[nrn_region]
                    ion_curr = "_ref_" + nrn_region + self._name
                    tenthousand_over_charge_faraday = 10000.0 / (
                        self._charge * h.FARADAY
                    )
                    my_dx = self._dx
                    # TODO: this better
                    if not isinstance(my_dx, tuple):
                        my_dx = (my_dx, my_dx, my_dx)
                    self._current_neuron_pointers = [
                        getattr(seg, ion_curr)
                        for seg in self._region._segs3d(
                            self._seg_to_surface_nodes.keys()
                        )
                    ]
                    # These are in the same order as self._surface_nodes_per_seg so self._surface_nodes_per_seg_start_indices will work for this list as well
                    geom_area = [
                        sum(self._region.geometry.surface_areas1d(sec))
                        for sec in self._region._secs3d
                    ]
                    node_area = [node.surface_area for node in self._nodes]
                    node_vol = [node.volume for node in self._nodes]
                    scale = geom_area / sum(node_area)
                    scale_factors = [
                        sign * area * scale * tenthousand_over_charge_faraday / vol
                        for area, vol in zip(node_area, node_vol)
                    ]
                    self._scale_factors = numpy.asarray(scale_factors, dtype=float)
                    _ics_set_grid_currents(
                        grid_list_start,
                        self._grid_id,
                        self._current_neuron_pointers,
                        self._scale_factors,
                    )

    def _semi_compile(self, region, instruction):
        self._isalive()
        if instruction == "do_3d":
            if self._species:
                sp = _defined_species[self._species][self._region]()
            else:
                for s in _all_species:
                    if self in s()._intracellular_instances.values():
                        sp = s()
                        break
            if isinstance(sp, Parameter):
                return "params_3d[%d]" % (self._grid_id)
            else:
                return "species_3d[%d]" % (self._grid_id)
        raise RxDException("_semi_compile received inconsistent state")

    def _register_cptrs(self):
        self._isalive()
        self._seg_order = []
        self._concentration_ptrs = []
        if self._nodes and self._species is not None:
            nrn_region = self._region._nrn_region
            if nrn_region is not None:
                ion = "_ref_" + self._species + nrn_region
                self._seg_order = list(self._region._nodes_by_seg.keys())
                for seg in self._region._segs3d(self._seg_order):
                    self._concentration_ptrs.append(getattr(seg, ion))

    def _import_concentration(self, init=True):
        self._isalive()
        nodes = self._nodes
        if not hasattr(self, "_seg_order"):
            self._register_cptrs()
        if nodes:
            # TODO: replace this with a pointer vec for speed
            #       not a huge priority since import happens rarely if at all
            conc_ptr = self._concentration_ptrs
            if self._region._nrn_region is not None:
                for segidx, ptr in zip(self._seg_order, conc_ptr):
                    value = ptr[0]
                    for node in self._region._nodes_by_seg[segidx]:
                        nodes[node].concentration = value

    def _transfer_to_legacy(self):
        self._isalive()
        nodes = self._nodes
        if self._nodes and self._region.nrn_region is not None:
            if self._region.nrn_region != "i":
                raise RxDException('only "i" nrn_region supported for 3D simulations')
                # TODO: remove this requirement
                #       the issue here is that the code below would need to keep track of which nodes are in which nrn_region
                #       that's not that big of a deal, but when this was coded, there were other things preventing this from working
        for segidx, ptr in zip(self._seg_order, self._concentration_ptrs):
            all_nodes_in_seg = list(self._region._nodes_by_seg[segidx])
            if all_nodes_in_seg:
                # TODO: if everything is 3D, then this should always have something, but for sections that aren't in 3D, won't have anything here
                # TODO: vectorize this, don't recompute denominator unless a structure change event happened
                ptr[0] = sum(
                    nodes[node].concentration * nodes[node].volume
                    for node in all_nodes_in_seg
                ) / sum(nodes[node].volume for node in all_nodes_in_seg)

    def _parse_diffusion(self, d):
        self._isalive()

        def get_signature(fun):
            import sys
            import inspect

            if sys.version_info.major > 2:
                sig = inspect.signature(fun)
                for param in sig.parameters.values():
                    if (
                        param.kind == param.VAR_POSITIONAL
                        or param.kind == param.VAR_KEYWORD
                    ):
                        raise RxDException(
                            "Intracellular diffusion coefficient function may not include *args or *kwargs"
                        )
                return len(sig.parameters)
            else:
                sig = inspect.getargspec(fun)
                if sig.varargs is not None or sig.keywords is not None:
                    raise RxDException(
                        "Intracellular diffusion coefficient function may not include *args or *kwargs"
                    )
                return len(sig.args)

        dc = None
        dgrid = None
        if callable(d) or (hasattr(d, "__len__") and callable(d[0])):
            dgrid = numpy.ndarray((3, self._nodes_length), dtype=float, order="C")
            if hasattr(d, "__len__"):
                if len(d) == 3:
                    for dr in range(3):
                        sig = get_signature(d[dr])
                        if sig == 3:
                            dgrid[dr, :] = [
                                d[dr](nd.x3d, nd.y3d, nd.z3d) for nd in self._nodes
                            ]
                        elif sig == 1:
                            dgrid[dr, :] = [d[dr](nd) for nd in self._nodes]
                else:
                    raise RxDException(
                        "Intracellular diffusion coefficient may be a scalar or a tuple of length 3 for anisotropic diffusion, it can also be a function for inhomogeneous diffusion (or tuple of 3 functions) with arguments x, y, z location or node with optional argument for direction"
                    )
            else:
                sig = get_signature(d)
                if sig == 4:
                    # x,y,z and direction
                    for dr in range(3):
                        dgrid[i, :] = [
                            d(nd.x3d, nd.y3d, nd.z3d, dr) for nd in self._nodes
                        ]

                elif sig == 3:
                    # x,y,z - isotropic
                    dg = [d(nd.x3d, nd.y3d, nd.z3d) for nd in self._nodes]
                    dgrid[0, :] = [d(nd.x3d, nd.y3d, nd.z3d) for nd in self._nodes]
                    dgrid[1, :] = dgrid[0, :]
                    dgrid[2, :] = dgrid[0, :]
                elif sig == 2:
                    # node and direction
                    for dr in range(3):
                        dgrid[dr, :] = [d(nd, dr) for nd in self._nodes]
                elif sig == 1:
                    # node - isotropic
                    dgrid[0, :] = [d(nd) for nd in self._nodes]
                    dgrid[1, :] = dgrid[0, :]
                    dgrid[2, :] = dgrid[0, :]
                else:
                    raise RxDException(
                        "Intracellular diffusion coefficient may be a scalar or a tuple of length 3 for anisotropic diffusion, it can also be a function for inhomogeneous diffusion (or tuple of 3 functions) with arguments x, y, z location or node with optional argument for direction"
                    )
        elif hasattr(d, "__len__"):
            if len(d) == 3:
                dc = numpy.array(d, dtype=float)
            elif len(d) == 1:
                dc = numpy.array(d[0] * numpy.ones(3), dtype=float)
            else:
                raise RxDException(
                    "Intracellular diffusion coefficient may be a scalar or a tuple of length 3 for anisotropic diffusion, it can also be a function for inhomogeneous diffusion (or tuple of 3 functions) with arguments x, y, z location or node with optional argument for direction"
                )
        else:
            dc = numpy.array(d * numpy.ones(3), dtype=float)
        return (dc, dgrid)

    @property
    def d(self):
        self._isalive()
        if hasattr(self, "_dgrid"):
            return self._dgrid
        return self._d

    @d.setter
    def d(self, value):
        self._isalive()
        dc, dgrid = self._parse_diffusion(value)
        if dc is not None:
            if hasattr(self, "_dgrid"):
                delattr(self, "_dgrid")
            self._d = dc
            set_diffusion(0, self._grid_id, dc, 1)
        else:
            self._dgrid = dgrid
            set_diffusion(0, self._grid_id, self._dgrid, self._nodes_length)

    # TODO Can we do this better?
    def _index_from_point(self, point):
        self._isalive()
        for i, n in enumerate(self._nodes):
            if point == (n._i, n._j, n._k):
                return i

    # TODO Can I do this better?
    def _mc3d_indices_start(self, r):
        self._isalive()
        indices = []
        for node_idx in r._nodes_by_seg.values():
            first_index = node_idx[0]
            point = r._points[first_index]
            indices.append(self._index_from_point(point))
        return min(indices)


class _ExtracellularSpecies(_SpeciesMathable):
    def __init__(
        self,
        region,
        d=0,
        name=None,
        charge=0,
        initial=0,
        atolscale=1.0,
        boundary_conditions=None,
        species=None,
    ):
        """
        region = Extracellular object (TODO? or list of objects)
        name = string of the name of the NEURON species (e.g. ca)
        d = diffusion constant
        charge = charge of the molecule (used for converting currents to concentration changes)
        initial = initial concentration
        alpha = volume fraction - either a single value for the whole region, a Vector giving a value for each voxel  or a anonymous function with argument of rxd.ExtracellularNode
        tortuosity = increase in path length due to obstacles, effective diffusion coefficient d/tortuosity^2. - either a single value for the whole region or a Vector giving a value for each voxel, or a anonymous function with argument of rxd.ExtracellularNode
        atolscale = scale factor for absolute tolerance in variable step integrations
        boundary_conditions = None by default corresponding to Neumann (zero flux) boundaries or a concentration for Dirichlet boundaries
        NOTE: For now, only define this AFTER the morphology is instantiated. In the future, this requirement can be relaxed.
        TODO: remove this limitation


        """
        _extracellular_diffusion_objects[self] = None

        # ensure 3D points exist
        h.define_shape()

        self._region = region
        self._species = name
        self._charge = charge
        self._xlo, self._ylo, self._zlo = region._xlo, region._ylo, region._zlo
        self._dx = region._dx
        self._d = d
        self._nx = region._nx
        self._ny = region._ny
        self._nz = region._nz
        self._xhi, self._yhi, self._zhi = region._xhi, region._yhi, region._zhi
        self._atolscale = atolscale
        self._states = h.Vector(self._nx * self._ny * self._nz)

        self.states = self._states.as_numpy().reshape(self._nx, self._ny, self._nz)
        self._initial = initial
        self._boundary_conditions = boundary_conditions
        self._diffusion_characteristic = (
            isinstance(region.alpha, Species) and region._alpha == species
        ) or (
            hasattr(region, "_permeability")
            and isinstance(region._permeability, Species)
            and region._permeability == species
        )
        if self._diffusion_characteristic:
            self.alpha = 1.0
            self._alpha = 1.0
        elif numpy.isscalar(region._volume_fraction_vector):
            self.alpha = self._alpha = region._volume_fraction_vector
        else:
            self.alpha = region.alpha
            self._alpha = region._volume_fraction_vector._ref_x[0]

        if self._diffusion_characteristic:
            self.tortuosity = 1.0
            self._permability = 1.0
        else:
            self.tortuosity = region.tortuosity
            if numpy.isscalar(region._permeability_vector):
                self._permability = region._permeability_vector
            else:
                self._permability = region._permeability_vector._ref_x[0]

        if boundary_conditions is None:
            bc_type = 0
            bc_value = 0.0
        else:
            bc_type = 1
            bc_value = boundary_conditions

        # TODO: if allowing different diffusion rates in different directions, verify that they go to the right ones
        if not hasattr(self._d, "__len__"):
            self._grid_id = ECS_insert(
                0,
                self._states._ref_x[0],
                self._nx,
                self._ny,
                self._nz,
                self._d,
                self._d,
                self._d,
                self._dx[0],
                self._dx[1],
                self._dx[2],
                self._alpha,
                self._permability,
                bc_type,
                bc_value,
                atolscale,
            )
        elif len(self._d) == 3:
            self._grid_id = ECS_insert(
                0,
                self._states._ref_x[0],
                self._nx,
                self._ny,
                self._nz,
                self._d[0],
                self._d[1],
                self._d[2],
                self._dx[0],
                self._dx[1],
                self._dx[2],
                self._alpha,
                self._permability,
                bc_type,
                bc_value,
                atolscale,
            )
        else:
            raise RxDException(
                "Diffusion coefficient %s for %s is invalid. A single value D or a tuple of 3 values (Dx,Dy,Dz) is required for the diffusion coefficient."
                % (repr(d), name)
            )

        self._name = name

        # set up the ion mechanism and enable active Nernst potential calculations
        self._ion_register()

        # moved to _finitialize -- in case the species is created before all the sections are
        # self._update_pointers()

        _defined_species_by_gid.append(self)

    def _isalive(self):
        if self not in _extracellular_diffusion_objects:
            raise RxDException("The extracellular species has been deleted.")

    def _delete(self):
        global _extracellular_diffusion_objects, _defined_species_by_gid
        # TODO: remove this object from the list of grids, possibly by reinserting all the others
        # NOTE: be careful about doing the right thing at program's end; some globals may no longer exist
        if self in _defined_species_by_gid:
            _defined_species_by_gid.remove(self)
        if _extracellular_diffusion_objects:
            if self in _extracellular_diffusion_objects:
                del _extracellular_diffusion_objects[self]
                # remove the grid id
                for sp in _extracellular_diffusion_objects:
                    if sp._grid_id > self._grid_id:
                        sp._grid_id -= 1
                _delete_by_id(self._grid_id)
                # remove any node.include_flux for the extracellular species.
                from . import node

                newflux = {
                    "index": [],
                    "type": [],
                    "source": [],
                    "scale": [],
                    "region": [],
                }
                for idx, t, src, sc, rptr in zip(
                    node._node_fluxes["index"],
                    node._node_fluxes["type"],
                    node._node_fluxes["source"],
                    node._node_fluxes["scale"],
                    node._node_fluxes["region"],
                ):
                    if t != self._grid_id:
                        if t > self._grid_id:
                            newflux["type"].append(t)
                        else:
                            newflux["type"].append(t - 1)
                        newflux["index"].append(idx)
                        newflux["type"].append(t)
                        newflux["source"].append(src)
                        newflux["scale"].append(sc)
                        newflux["region"].append(region)
                node._has_node_fluxes = newflux != node._node_fluxes
                node._node_fluxes = newflux
                nrn_dll_sym("structure_change_cnt", ctypes.c_int).value += 1

    def _finitialize(self):
        self._isalive()
        # Updated - now it will initialize using NodeExtracellular
        if self._initial is None:
            if hasattr(h, "%so0_%s_ion" % (self._species, self._species)):
                self.states[:] = getattr(
                    h, "%so0_%s_ion" % (self._species, self._species)
                )
            else:
                self.states[:] = 0
        if self._boundary_conditions:
            for idx in [0, -1]:
                self.states[idx, :, :] = self._boundary_conditions
                self.states[:, idx, :] = self._boundary_conditions
                self.states[:, :, idx] = self._boundary_conditions
        # necessary if section where created after the _ExtracellularSpecies
        self._update_pointers()

    def _ion_register(self):
        """modified from neuron.rxd.species.Species._ion_register"""

        self._isalive()
        if self._species is not None:
            ion_type = h.ion_register(self._species, self._charge)
            if ion_type == -1:
                raise RxDException("Unable to register species: %s" % self._species)
            # insert the species if not already present
            ion_forms = [
                self._species + "i",
                self._species + "o",
                "i" + self._species,
                "e" + self._species,
            ]
            for s in h.allsec():
                try:
                    for i in ion_forms:
                        # this throws an exception if one of the ion forms is missing
                        temp = getattr(s, i)
                except:
                    s.insert(self._species + "_ion")
                # set to recalculate reversal potential automatically
                # the last 1 says to set based on global initial concentrations
                # e.g. nao0_na_ion, etc...
                # TODO: this is happening GLOBALLY even to sections that aren't inside the extracellular domain (FIX THIS)
                h.ion_style(self._species + "_ion", 3, 2, 1, 1, 1, sec=s)

    def _nodes_by_location(self, i, j, k):
        self._isalive()
        return (i * self._ny + j) * self._nz + k

    def index_from_xyz(self, x, y, z):
        """Given an (x, y, z) point, return the index into the _species vector containing it or None if not in the domain"""

        self._isalive()
        if (
            x < self._xlo
            or x > self._xhi
            or y < self._ylo
            or y > self._yhi
            or z < self._zlo
            or z > self._zhi
        ):
            return None
        # if we make it here, then we are inside the domain
        i = int((x - self._xlo) / self._dx[0])
        j = int((y - self._ylo) / self._dx[1])
        k = int((z - self._zlo) / self._dx[2])
        return self._nodes_by_location(i, j, k)

    def ijk_from_index(self, index):
        self._isalive()
        nynz = self._ny * self._nz
        i = int(index / nynz)
        jk = index - nynz * i
        j = int(jk / self._nz)
        k = jk % self._nz
        # sanity check
        assert index == self._nodes_by_location(i, j, k)
        return i, j, k

    def xyz_from_index(self, index):
        self._isalive()
        i, j, k = ijk_from_index(self, index)
        return (
            self._xlo + i * self._dx[0],
            self._ylo + j * self._dx[1],
            self._zlo + k * self._dx[2],
        )

    def _locate_segments(self):
        """Note: there may be Nones in the result if a section extends outside the extracellular domain

        Note: the current version keeps all the sections alive (i.e. they will never be garbage collected)
        TODO: fix this
        """

        self._isalive()
        result = {}
        for sec in h.allsec():
            result[sec] = [self.index_from_xyz(*_xyz(seg)) for seg in sec]
        return result

    def _update_pointers(self):
        # TODO: call this anytime the _grid_id changes and anytime the structure_change_count change
        self._isalive()
        if self._species is None:
            return
        self._seg_indices = self._locate_segments()
        from .geometry import _surface_areas1d

        grid_list = 0
        grid_indices = []
        neuron_pointers = []
        stateo = "_ref_" + self._species + "o"
        for sec, indices in self._seg_indices.items():
            for seg, i in zip(sec, indices):
                if i is not None:
                    grid_indices.append(i)
                    neuron_pointers.append(getattr(seg, stateo))
        _set_grid_concentrations(
            grid_list, self._grid_id, grid_indices, neuron_pointers
        )
        if self._charge == 0 or isinstance(
            _defined_species[self._species][self._region](), Parameter
        ):
            _set_grid_currents(grid_list, self._grid_id, [], [], [])
        else:
            tenthousand_over_charge_faraday = 10000.0 / (self._charge * h.FARADAY)
            scale_factor = tenthousand_over_charge_faraday / (numpy.prod(self._dx))
            ispecies = "_ref_i" + self._species
            neuron_pointers = []
            scale_factors = []
            for sec, indices in self._seg_indices.items():
                for seg, surface_area, i in zip(sec, _surface_areas1d(sec), indices):
                    if i is not None:
                        neuron_pointers.append(getattr(seg, ispecies))
                        scale_factors.append(float(scale_factor * surface_area))
            # TODO: MultiCompartment reactions ?
            _set_grid_currents(
                grid_list, self._grid_id, grid_indices, neuron_pointers, scale_factors
            )

    def _import_concentration(self):
        """set node values to those of the corresponding NEURON segments"""

        self._isalive()
        if self._species is None:
            return
        grid_list = 0
        grid_indices = []
        neuron_pointers = []
        stateo = self._species + "o"
        for sec, indices in self._seg_indices.items():
            for seg, i in zip(sec, indices):
                if i is not None:
                    self._states[i] = getattr(seg, stateo)

    def _semi_compile(self, reg, instruction):

        self._isalive()
        if self._species:
            sp = _defined_species[self._species][self._region]()
        else:
            for s in _all_species:
                if self in s()._extracellular_instances.values():
                    sp = s()
                    break
        if isinstance(sp, Parameter):
            return "params_3d[%d]" % (self._grid_id)
        else:
            return "species_3d[%d]" % (self._grid_id)

    @property
    def d(self):

        self._isalive()
        return self._d

    @d.setter
    def d(self, value):
        self._isalive()
        if self._d != value:
            self._d = value
            if hasattr(value, "__len__"):
                set_diffusion(0, self._grid_id, numpy.array(value, dtype=float), 1)
            else:
                set_diffusion(0, self._grid_id, numpy.repeat(float(value), 3), 1)


# TODO: make sure that we can make this work where things diffuse across the
#       boundary between two regions... for the tree solver, this is complicated
#       because need the sections in a sorted order
#       ... but this is also weird syntactically, because how should it know
#       that two regions (e.g. apical and other_dendrites) are connected?


class Species(_SpeciesMathable):
    def __init__(
        self,
        regions=None,
        d=0,
        name=None,
        charge=0,
        initial=None,
        atolscale=1,
        ecs_boundary_conditions=None,
        represents=None,
    ):
        """s = rxd.Species(regions, d = 0, name = None, charge = 0, initial = None, atolscale=1)

        Declare a species.

        Parameters:

        regions -- a Region or list of Region objects containing the species

        d -- the diffusion constant of the species (optional; default is 0, i.e. non-diffusing)

        name -- the name of the Species; used for syncing with NMODL and HOC (optional; default is none)

        charge -- the charge of the Species (optional; default is 0)

        initial -- the initial concentration or None (if None, then imports from HOC if the species is defined at finitialize, else 0)

        atolscale -- scale factor for absolute tolerance in variable step integrations

        ecs_boundary_conditions  -- if Extracellular rxd is used ecs_boundary_conditions=None for zero flux boundaries or if ecs_boundary_conditions=the concentration at the boundary.

        represents -- optionally provide CURIE (Compact URI) to annotate what the species represents e.g. CHEBI:29101 for sodium(1+)

        Note:

        charge must match the charges specified in NMODL files for the same ion, if any.

        You probably want to adjust atolscale for species present at low concentrations (e.g. calcium).
        """

        import neuron
        import ctypes

        from . import rxd, region

        # if there is a species, then rxd is being used, so we should register
        # this function may be safely called many times
        rxd._do_nbs_register()

        # TODO: check if "name" already defined elsewhere (if non-None)
        #       if so, make sure other fields consistent, expand regions as appropriate

        self._allow_setting = True
        self.regions = regions
        # Check for anisotropic/inhomogeneous 3D diffusion with 1D sections
        if hasattr(d, "__len__") or callable(d):
            if (
                hasattr(d, "__len__")
                and all([di == d[0] for di in d])
                and not callable(d[0])
            ):
                self._d = d[0]
            else:
                from .rxd import _domain_lookup

                reglist = regions if hasattr(regions, "__len__") else [regions]
                dims = [
                    _domain_lookup(sec)
                    for reg in reglist
                    if not isinstance(reg, region.Extracellular)
                    for sec in reg.secs
                ]
                if not all([dim == dims[0] for dim in dims]):
                    raise RxDException(
                        "Hybrid 1D/3D diffusion does not currently support anisotropy or inhomogeneous grids. For separate 1D and 3D diffusion please create separate regions and species for 3D and 1D sections"
                    )
                else:
                    self._d = d
        else:
            self._d = d
        self.name = name
        self.charge = charge
        self.initial = initial
        self._atolscale = atolscale
        self._ecs_boundary_conditions = ecs_boundary_conditions
        self._species_on_region = weakref.WeakKeyDictionary()
        if represents and not _ontology_id.match(represents):
            raise RxDException("The represents=%s is not valid CURIE" % represents)
        else:
            self.represents = represents
        with initializer._init_lock:
            _all_species.append(weakref.ref(self))

            # declare an update to the structure of the model (the number of differential equations has changed)
            nrn_dll_sym("structure_change_cnt", ctypes.c_int).value += 1

            self._ion_register()

            # initialize self if the rest of rxd is already initialized
            if initializer.is_initialized():
                self._do_init()
                rxd._update_node_data(True, True)

    def _do_init(self):
        self._do_init1()
        self._do_init2()
        self._do_init3()
        self._do_init4()
        self._do_init5()
        self._do_init6()

    def _do_init1(self):
        from . import rxd

        # TODO: if a list of sections is passed in, make that one region
        # _species_count is used to create a unique _real_name for the species
        global _species_count

        regions = self.regions
        self._real_name = self._name
        initial = self.initial
        charge = self._charge

        # invalidate any old initialization of external solvers
        rxd._external_solver_initialized = False

        # TODO: check about the 0<x<1 problem alluded to in the documentation
        h.define_shape()
        name = self.name
        if name is not None:
            if not isinstance(name, str):
                raise RxDException("Species name must be a string")
            if name in _defined_species:
                spsecs_i = []
                spsecs_o = []
                for r in regions:
                    if r in _defined_species[name]:
                        raise RxDException(
                            'Species "%s" previously defined on region: %r' % (name, r)
                        )
                    if hasattr(r, "_secs"):
                        if r.nrn_region == "i":
                            spsecs_i += r._secs
                        elif r.nrn_region == "o":
                            spsecs_o += r._secs
                spsecs_i = set(spsecs_i)
                spsecs_o = set(spsecs_o)
                for r in _defined_species[name]:
                    if hasattr(r, "_secs") and r.nrn_region:
                        if (
                            r.nrn_region == "i"
                            and any(spsecs_i.intersection(r._secs))
                            or (
                                r.nrn_region == "o"
                                and any(spsecs_o.intersection(r._secs))
                            )
                        ):
                            raise RxDException(
                                'Species "%s" previously defined on a region %r that overlaps with regions: %r'
                                % (name, r, self._regions)
                            )
        else:
            name = _species_count
        self._id = _species_count
        _species_count += 1
        if name not in _defined_species:
            _defined_species[name] = weakref.WeakKeyDictionary()
        self._species = weakref.ref(self)
        for r in regions:
            _defined_species[name][r] = self._species
        _all_defined_species.append(self._species)

        if regions is None:
            raise RxDException("Must specify region where species is present")
        if hasattr(regions, "__len__"):
            regions = list(regions)
        else:
            regions = list([regions])
        # TODO: unite handling of _regions and _extracellular_regions
        self._regions = [r for r in regions if not isinstance(r, region.Extracellular)]
        self._extracellular_regions = [
            r for r in regions if isinstance(r, region.Extracellular)
        ]
        if not all(isinstance(r, region.Region) for r in self._regions):
            raise RxDException(
                "regions list must consist of Region and Extracellular objects only"
            )

        # at this point self._name is None if unnamed or a string == name if
        # named
        self._ion_register()

        # TODO: remove this line when certain no longer need it (commented out 2013-04-17)
        # self._real_secs = region._sort_secs(sum([r.secs for r in regions], []))

    def _do_init2(self):
        # 1D section objects; NOT all sections this species lives on
        # TODO: this may be a problem... debug thoroughly
        global _has_1d
        d = self._d

        self._secs = [
            Section1D(self, sec, d, r) for r in self._regions for sec in r._secs1d
        ]
        if self._secs:
            self._offset = self._secs[0]._offset
            _has_1d = True
        else:
            self._offset = 0
        self._has_adjusted_offsets = False
        self._assign_parents()

    def _do_init3(self):
        global _has_3d
        # 3D sections

        # NOTE: if no 3D nodes, then _3doffset is not meaningful
        self._3doffset = 0
        self._3doffset_by_region = {}
        selfref = weakref.ref(self)
        self_has_3d = False
        self._intracellular_nodes = {}
        if self._regions:
            for r in self._regions:
                self._intracellular_nodes[r] = []
                if any(r._secs3d):
                    xs, ys, zs, segsidx = r._xs, r._ys, r._zs, r._segsidx
                    segs = [seg for sec in r._secs3d for seg in sec]
                    self._intracellular_nodes[r] += [
                        node.Node3D(i, x, y, z, r, self._d, segs[idx], selfref)
                        for i, x, y, z, idx in zip(range(len(xs)), xs, ys, zs, segsidx)
                    ]
                    # the region is now responsible for computing the correct volumes and surface areas
                    # this is done so that multiple species can use the same region without recomputing it
                    self_has_3d = True
                    _has_3d = True
        is_diffusable = (
            False if isinstance(self, Parameter) or isinstance(self, State) else True
        )
        self._intracellular_instances = {
            r: _IntracellularSpecies(
                r,
                d=self._d,
                charge=self.charge,
                initial=self.initial,
                nodes=self._intracellular_nodes[r],
                name=self._name,
                is_diffusable=is_diffusable,
                atolscale=self._atolscale,
            )
            for r in self._regions
            if any(r._secs3d)
        }

    def _do_init4(self):
        extracellular_nodes = []
        self._extracellular_instances = {
            r: _ExtracellularSpecies(
                r,
                d=self._d,
                name=self.name,
                charge=self.charge,
                initial=self.initial,
                atolscale=self._atolscale,
                boundary_conditions=self._ecs_boundary_conditions,
                species=self,
            )
            for r in self._extracellular_regions
        }
        sp_ref = weakref.ref(self)
        index = 0
        for r in self._extracellular_regions:
            r_ref = weakref.ref(r)
            for i in range(r._nx):
                for j in range(r._ny):
                    extracellular_nodes += [
                        node.NodeExtracellular(idx, i, j, k, sp_ref, r_ref)
                        for k, idx in enumerate(range(index, index + r._nz))
                    ]
                    index += r._nz
            # extracellular_nodes += [node.NodeExtracellular(idx, i, j, k, sp_ref, r_ref) for i, idxx in  enumerate(range(index, index + r._nx * r._ny * r._nz, r._ny * r._nz)) for j, idxy in enumerate(range(idxx, idxx + r._nz * r._ny, r._nz)) for k,idx in enumerate(range(idxy, idxy + r._nz))]
            # index = len(extracellular_nodes)
        self._extracellular_nodes = extracellular_nodes

    def _do_init5(self):
        # final initialization
        for sec in self._secs:
            # NOTE: can only init_diffusion_rates after the roots (parents)
            #       have been assigned
            sec._init_diffusion_rates()
        self._update_node_data()  # is this really nessesary
        self._update_region_indices()

    def _do_init6(self):
        self._register_cptrs()

    def __del__(self):
        global _all_species, _defined_species, _all_defined_species
        if hasattr(self, "deleted"):
            return
        self.deleted = True
        if hasattr(self, "_id"):
            remove_species_atolscale(self._id)

        name = self.name if self.name else (self._id if hasattr(self, "_id") else None)
        if name is not None and name in _defined_species:
            for r in self.regions:
                if r in _defined_species[name]:
                    del _defined_species[name][r]
            if not any(_defined_species[name]):
                del _defined_species[name]
        _all_defined_species = list(
            filter(lambda x: x() is not None and x() is not self, _all_defined_species)
        )
        _all_species = list(
            filter(lambda x: x() is not None and x() is not self, _all_species)
        )
        if hasattr(self, "_extracellular_instances"):
            for sp in self._extracellular_instances.values():
                sp._delete()

        if hasattr(self, "_intracellular_instances"):
            for sp in self._intracellular_instances.values():
                sp._delete()

        # delete the secs
        if hasattr(self, "_secs") and self._secs:
            # remove the species root
            # TODO: will this work with post initialization morphology changes
            self._secs.sort(key=lambda s: s._offset, reverse=True)
            for sec in self._secs[:]:
                sec._delete()
            self._secs = []

        # set the remaining species offsets
        if initializer.is_initialized():
            for sr in _all_species:
                s = sr()
                if s is not None:
                    s._update_region_indices(True)

    def _ion_register(self):
        charge = self._charge
        if self._name is not None:
            ion_type = None
            # insert the species if not already present
            regions = (
                self._regions if hasattr(self._regions, "__len__") else [self._regions]
            )
            for r in regions:
                if r.nrn_region in ("i", "o"):
                    if ion_type is None:
                        ion_type = h.ion_register(self._name, charge)
                        if ion_type == -1:
                            raise RxDException(
                                "Unable to register species: %s" % self._name
                            )

                    for s in r.secs:
                        try:
                            ion_forms = [
                                self._name + "i",
                                self._name + "o",
                                "i" + self._name,
                                "e" + self._name,
                            ]
                            for i in ion_forms:
                                # this throws an exception if one of the ion forms is missing
                                temp = getattr(s, self._name + "i")
                        except:
                            s.insert(self._name + "_ion")
                        # set to recalculate reversal potential automatically
                        # the last 1 says to set based on global initial concentrations
                        # e.g. nai0_na_ion, etc...
                        h.ion_style(self._name + "_ion", 3, 2, 1, 1, 1, sec=s)

    @property
    def states(self):
        """A list of all the state values corresponding to this species"""
        all_states = node._get_states()
        return [all_states[i] for i in numpy.sort(self.indices())]

    @property
    def _state(self):
        """return a bytestring representing the Species state"""
        # format: version identifier (unsigned long long), size (unsigned long long), binary data
        import array

        version = 0
        data = array.array("d", self.nodes.concentration).tobytes()
        return array.array("Q", [version, len(data)]).tobytes() + data

    @_state.setter
    def _state(self, oldstate):
        """restore Species state"""
        import array

        metadata_array = array.array("Q")
        metadata_array.frombytes(oldstate[:16])
        version, length = metadata_array
        if version != 0:
            raise RxdException("Unsupported state data version")
        data = array.array("d")
        try:
            data.frombytes(oldstate[16:])
        except ValueError:
            # happens when not a multiple of 8 bytes
            raise RxDException("Invalid state data length") from None
        # at 8 bytes per data point, the total number of bytes should match the stored
        if len(data) * 8 != length or len(data) != len(self.nodes):
            raise RxDException("Invalid state data length")
        self.nodes.concentration = data

    def re_init(self):
        """Reinitialize the rxd concentration of this species to match the NEURON grid"""
        self._import_concentration(init=False)

    def __getitem__(self, r):
        """Return a reference to those members of this species lying on the specific region @varregion.
        The resulting object is a SpeciesOnRegion.
        This is useful for defining reaction schemes for MultiCompartmentReaction."""
        if isinstance(r, region.Region) and r in self._regions:
            if r not in self._species_on_region:
                self._species_on_region[r] = SpeciesOnRegion(self, r)
            return self._species_on_region[r]
        elif isinstance(r, region.Extracellular):
            if not hasattr(self, "_extracellular_instances"):
                initializer._do_init()
            if r in self._extracellular_instances:
                if r not in self._species_on_region:
                    self._species_on_region[r] = SpeciesOnExtracellular(
                        self, self._extracellular_instances[r]
                    )
                return self._species_on_region[r]
        raise RxDException("no such region")

    def _update_node_data(self):
        nsegs_changed = 0
        for sec in self._secs:
            nsegs_changed += sec._update_node_data()
        # remove deleted sections
        self._secs = [sec for sec in self._secs if sec and sec._nseg > 0]

        # check if 3D sections/nseg has changed
        for r in self._intracellular_instances:
            if r._secs3d_names:
                secs3d_names = {sec.hoc_internal_name(): sec.nseg for sec in r._secs3d}
                if secs3d_names:
                    if secs3d_names != r._secs3d_names:
                        # TODO: redo voxelization and interpolate
                        raise RxDException(
                            "Error: changing the 3D sections or their nseg after initialization is not yet supported."
                        )
                else:
                    # If there are no 3D sections we can continue without
                    # re-voxelization.
                    self._intracellular_nodes[r] = []

        return nsegs_changed

    def concentrations(self):
        if self._secs:
            raise RxDException("concentrations only supports 3d and that is deprecated")
        warnings.warn("concentrations is deprecated; do not use")
        r = self._regions[0]
        data = numpy.array(r._mesh.values, dtype=float)
        # things outside of the volume will be NaN
        data[:] = numpy.NAN
        max_concentration = -1
        for node in self.nodes:
            data[node._i, node._j, node._k] = node.concentration
        return data

    def _register_cptrs(self):
        # 1D stuff
        for sec in self._secs:
            sec._register_cptrs()
        idx = self._indices1d()
        species_atolscale(
            self._id, self._atolscale, len(idx), (ctypes.c_int * len(idx))(*idx)
        )
        # 3D stuff
        for ics in self._intracellular_instances.values():
            ics._register_cptrs()

    @property
    def charge(self):
        """Get or set the charge of the Species.

        .. note:: Setting was added in NEURON 7.4+ and is allowed only before the reaction-diffusion model is instantiated.
        """
        return self._charge

    @charge.setter
    def charge(self, value):
        if hasattr(self, "_allow_setting"):
            self._charge = value
        else:
            raise RxDException("Cannot set charge now; model already instantiated")

    @property
    def regions(self):
        """Get or set the regions where the Species is present

        .. note:: New in NEURON 7.4+. Setting is allowed only before the reaction-diffusion model is instantiated.

        .. note:: support for getting the regions is new in NEURON 7.5.
        """
        return list(self._regions) + list(self._extracellular_regions)

    @regions.setter
    def regions(self, regions):
        if hasattr(self, "_allow_setting"):
            if hasattr(regions, "__len__"):
                regions = list(regions)
            else:
                regions = list([regions])
            self._regions = [
                r for r in regions if not isinstance(r, region.Extracellular)
            ]
            self._extracellular_regions = [
                r for r in regions if isinstance(r, region.Extracellular)
            ]
        else:
            raise RxDException("Cannot set regions now; model already instantiated")

    def _update_region_indices(self, update_offsets=False):
        # TODO: should this include 3D nodes? currently 1D only. (What faces the user?)
        # update offset in case nseg has changed
        self._region_indices = {}
        if hasattr(self, "_secs") and self._secs:
            if update_offsets:
                self._offset = self._secs[0]._offset - self._num_roots
            for s in self._secs:
                if s._region not in self._region_indices:
                    self._region_indices[s._region] = []
                self._region_indices[s._region] += s.indices
        # a list of all indices
        self._region_indices[None] = list(
            itertools.chain.from_iterable(list(self._region_indices.values()))
        )

    def _indices3d(self, r=None):
        """return the indices of just the 3D nodes corresponding to this species in the given region"""
        # TODO: Remove this
        return []
        # TODO: this will need changed if 3D is to support more than one region
        if r is None or r == self._regions[0]:
            return list(range(self._3doffset, self._3doffset + len(self._nodes)))
        else:
            return []

    def _indices1d(self, r=None, secs=None):
        """return the indices of just the 1D nodes corresponding to this species in the given region"""
        if secs is not None:
            if r is not None:
                return [
                    idx
                    for sec in self._secs
                    for idx in sec.indices
                    if sec in secs and sec._region == r
                ]
            return [idx for sec in self._secs for idx in sec.indices if sec in secs]
        return self._region_indices.get(r, [])

    def indices(self, r=None, secs=None):
        """return the indices corresponding to this species in the given region

        if r is None, then returns all species indices
        If r is a list of regions return indices for only those sections that are on all the regions.
        If secs is a set return all indices on the regions for those sections."""
        # TODO: beware, may really want self._indices3d or self._indices1d
        initializer._do_init()
        if secs is not None:
            if type(secs) != set:
                secs = {secs}
            if r is None:
                regions = self._regions
            elif not hasattr(r, "__len__"):
                regions = [r]
            else:
                regions = r
            return list(
                itertools.chain.from_iterable(
                    [
                        s.indices
                        for s in self._secs
                        if s._sec in secs and s.region in regions
                    ]
                )
            )

            """for reg in regions:
                ind = self.indices(reg)
                offset = 0
                for sec in r.secs:
                    if sec not in secs:
                        del ind[offset:(offset+sec.nseg)]
                    offset += sec.nseg
                return ind"""
        else:
            if not hasattr(r, "__len__"):
                return self._indices1d(r) + self._indices3d(r)
            elif len(r) == 1:
                return self._indices1d(r[0]) + self._indices3d(r[0])
            else:  # Find the intersection
                interseg = set.intersection(
                    *[set(reg.secs) for reg in r if reg is not None]
                )
                return self.indices(r, interseg)

    def defined_on_region(self, r):
        return r in self._regions or r in self._extracellular_regions

    def _setup_diffusion_matrix(self, g):
        for s in self._secs:
            s._setup_diffusion_matrix(g)

    def _setup_c_matrix(self, c):
        # TODO: this will need to be changed for three dimensions, or stochastic
        for s in self._secs:
            c[s._offset : s._offset + s.nseg] = 1.0

    def _setup_currents(self, indices, scales, ptrs, cur_map):
        from . import rxd

        if self.name:
            if self.name + "i" not in cur_map:
                cur_map[self.name + "i"] = {}
            if self.name + "o" not in cur_map:
                cur_map[self.name + "o"] = {}
        # 1D part
        for s in self._secs:
            s._setup_currents(indices, scales, ptrs, cur_map)

    def _has_region_section(self, region, sec):
        return any((s._region == region and s._sec == sec) for s in self._secs)

    def _region_section(self, region, sec):
        """return the Section1D (or whatever) associated with the region and section"""
        for s in self._secs:
            if s._region == region and s._sec == sec:
                return s
        else:
            raise RxDException("requested section not in species")

    def _assign_parents(self):
        root_id = 0
        missing = {}
        self._root_children = []
        for sec in self._secs:
            root_id = sec._assign_parents(root_id, missing, self._root_children)
        self._num_roots = root_id
        # TODO: this probably doesn't do the right thing if the morphology
        #       is changed after creation
        # adjust offsets to account for roots
        if not self._has_adjusted_offsets:
            node._allocate(self._num_roots)
            for sec in self._secs:
                sec._offset += root_id
            # used for section1d.__del__
            if self._secs != []:
                self._secs[0]._num_roots = self._num_roots
            self._has_adjusted_offsets = True

    def _finitialize(self, skip_transfer=False):
        if self._extracellular_instances:
            for r in self._extracellular_instances.keys():
                self._extracellular_instances[r]._finitialize()
        if self._intracellular_instances:
            for r in self._intracellular_instances.keys():
                self._intracellular_instances[r]._finitialize()
        if self.initial is not None:
            if isinstance(self.initial, Callable):
                for node in self.nodes:
                    node.concentration = self.initial(node)
            else:
                for node in self.nodes:
                    node.concentration = self.initial
            if not skip_transfer:
                self._transfer_to_legacy()
        else:
            self._import_concentration()

    def _transfer_to_legacy(self):
        """Transfer concentrations to the standard NEURON grid"""
        if self._name is None:
            return

        # 1D part
        for sec in self._secs:
            sec._transfer_to_legacy()

        # now the 3D stuff
        for ics in self._intracellular_instances.values():
            ics._transfer_to_legacy()

        """
        nodes = self._nodes
        if nodes:
            non_none_regions = [r for r in self._regions if r._nrn_region is not None]
            if non_none_regions:
                if any(r._nrn_region != 'i' for r in non_none_regions):
                    raise RxDException('only "i" nrn_region supported for 3D simulations')
                    # TODO: remove this requirement
                    #       the issue here is that the code below would need to keep track of which nodes are in which nrn_region
                    #       that's not that big of a deal, but when this was coded, there were other things preventing this from working                    
                for seg, ptr in zip(self._seg_order, self._concentration_ptrs):
                    all_nodes_in_seg = list(itertools.chain.from_iterable(r._surface_nodes_by_seg[seg] for r in non_none_regions))
                    if all_nodes_in_seg:
                        # TODO: if everything is 3D, then this should always have something, but for sections that aren't in 3D, won't have anything here
                        # TODO: vectorize this, don't recompute denominator unless a structure change event happened
                        ptr[0] = sum(nodes[node].concentration * nodes[node].volume for node in all_nodes_in_seg) / sum(nodes[node].volume for node in all_nodes_in_seg)
        """

    def _import_concentration(self, init=True):
        """Read concentrations from the standard NEURON grid"""
        if self._name is None:
            return

        # start with the 1D stuff
        for sec in self._secs:
            sec._import_concentration(init)

        # now the 3D stuff
        for ics in self._intracellular_instances.values():
            ics._import_concentration()

        # now the ECS
        for ecs in self._extracellular_instances.values():
            ecs._import_concentration()

    @property
    def nodes(self):
        """A NodeList of all the nodes corresponding to the species.

        This can then be further restricted using the callable property of NodeList objects."""

        from . import rxd

        _nrn_shape_update()
        if initializer.is_initialized():
            rxd._update_node_data()
        else:
            initializer._do_init()

        self._all_intracellular_nodes = []
        if self._intracellular_nodes:
            for r in self._regions:
                if r in self._intracellular_nodes:
                    self._all_intracellular_nodes += self._intracellular_nodes[r]
        # The first part here is for the 1D -- which doesn't keep live node objects -- the second part is for 3D
        self._all_intracellular_nodes = [
            nd for nd in self._all_intracellular_nodes[:] if nd.sec
        ]
        return nodelist.NodeList(
            list(itertools.chain.from_iterable([s.nodes for s in self._secs]))
            + self._all_intracellular_nodes
            + self._extracellular_nodes
        )

    @property
    def name(self):
        """Get or set the name of the Species.

        This is used only for syncing with the non-reaction-diffusion parts of NEURON.

        .. note:: Setting only supported in NEURON 7.4+, and then only before the reaction-diffusion model is instantiated.
        """
        return self._name

    @name.setter
    def name(self, value):
        if hasattr(self, "_allow_setting"):
            self._name = value
        else:
            raise RxDException("Cannot set name now; model already instantiated")

    def __repr__(self):
        represents = ", represents=%s" % self.represents if self.represents else ""
        return "Species(regions=%r, d=%r, name=%r, charge=%r, initial=%r%s)" % (
            self._regions,
            self._d,
            self._name,
            self._charge,
            self.initial,
            represents,
        )

    def _short_repr(self):
        if self._name is not None:
            return str(self._name)
        else:
            return self.__repr__()

    def __str__(self):
        if self._name is None:
            return self.__repr__()
        return str(self._name)


def xyz_by_index(indices):
    """Return the 3D location of the nodes at the given indices"""
    if hasattr(indices, "count"):
        index = indices
    else:
        index = [indices]
    # TODO: make sure to include Node3D
    return [
        [nd.x3d, nd.y3d, nd.z3d]
        for sp in _get_all_species()
        for s in sp()._secs
        for nd in s.nodes
        if sp()
        if nd._index in index
    ]


class Parameter(Species):
    """
    s = rxd.Parameter(regions, name=None, charge=0, value=None, represents=None)

    Declare a parameter, it can be used in place of a rxd.Species, but unlike rxd.Species a parameter will not change.

    Parameters:
    regions -- a Region or list of Region objects containing the parameter

    name -- the name of the parameter; used for syncing with NMODL and HOC (optional; default is none)

    charge -- the charge of the Parameter (optional; default is 0)

    value -- the value or None (if None, then imports from HOC if the parameter is defined at finitialize, else 0)

    represents -- optionally provide CURIE (Compact URI) to annotate what the parameter represents e.g. CHEBI:29101 for sodium(1+)

    Note:
    charge must match the charges specified in NMODL files for the same ion, if any.
    """

    def __init__(self, *args, **kwargs):
        if "value" in kwargs:
            if (
                "initial" in kwargs
                and kwargs["initial"]
                and kwargs["initial"] != kwargs["value"]
            ):
                raise RxDException(
                    "Parameter cannot be assigned both a 'value=%g' and 'initial=%g'"
                    % (kwargs["value"], kwargs["initial"])
                )
            kwargs["initial"] = kwargs.pop("value")
        if "d" in kwargs and kwargs["d"] != 0:
            raise RxDException(
                "Parameters cannot diffuse, invalid keyword argument 'd=%g'"
                % kwargs["d"]
            )
        self._d = 0
        super(Parameter, self).__init__(*args, **kwargs)

    def _semi_compile(self, reg, instruction):
        from . import region

        # region is Extracellular
        if isinstance(reg, region.Extracellular):
            ecs_instance = self._extracellular_instances[reg]
            return ecs_instance._semi_compile(reg, instruction)
        # region is 3d intracellular
        if isinstance(reg, region.Region) and instruction == "do_3d":
            ics_instance = self._intracellular_instances[reg]
            return ics_instance._semi_compile(reg, instruction)
        if isinstance(reg, region.Region) and instruction == "do_1d":
            if reg in self._regions:
                return "params[%d][%d]" % (self._id, reg._id)
            elif len(self._regions) == 1:
                return "params[%d][%d]" % (self._id, self._regions[0]._id)
            else:
                raise RxDException(
                    "Parameter %r is not defined on region %r" % (self, reg)
                )
        raise RxDException("_semi_compile received inconsistent state")

    def __repr__(self):
        represents = ", represents=%s" % self.represents if self.represents else ""
        return "Parameter(regions=%r, name=%r, charge=%r, value=%r%s)" % (
            self._regions,
            self._name,
            self._charge,
            self.initial,
            represents,
        )

    @property
    def value(self):
        return self.initial

    @value.setter
    def value(self, v):
        self.initial = v

    @property
    def d(self):
        return 0

    @d.setter
    def d(self, value):
        if value != 0:
            raise RxDException("Parameters cannot diffuse.")

    def __getitem__(self, r):
        """Return a reference to those members of this parameter lying on the specific region @varregion.
        The resulting object is a ParameterOnRegion or ParameterOnExtracellular.
        This is useful for defining reaction schemes for MultiCompartmentReaction."""
        if isinstance(r, region.Region) and r in self._regions:
            if r not in self._species_on_region:
                self._species_on_region[r] = ParameterOnRegion(self, r)
            return self._species_on_region[r]
        elif isinstance(r, region.Extracellular):
            if not hasattr(self, "_extracellular_instances"):
                initializer._do_init()
            if r in self._extracellular_instances:
                if r not in self._species_on_region:
                    self._species_on_region[r] = ParameterOnExtracellular(
                        self, self._extracellular_instances[r]
                    )
                return self._species_on_region[r]
        raise RxDException("no such region")


class ParameterOnRegion(SpeciesOnRegion):
    def _semi_compile(self, reg, instruction):
        """reg = self._region()
        if reg._secs3d:
            ics_instance = self._species()._intracellular_instances[reg]
            return ics_instance._semi_compile(reg, instruction)
        elif reg._secs1d:
            return 'params[%d][%d]' % (self._id, self._region()._id)"""
        reg = self._region()
        if instruction == "do_3d":
            ics_instance = self._species()._intracellular_instances[reg]
            return ics_instance._semi_compile(reg, instruction)
        elif any(reg._secs1d):
            return "params[%d][%d]" % (self._id, self._region()._id)
        else:
            raise RxDException(
                "Instruction for ParameterOnRegion semi_compile was neither 1d or 3d"
            )


class ParameterOnExtracellular(SpeciesOnExtracellular):
    pass


class State(Species):
    def __repr__(self):
        return "State(regions=%r, d=%r, name=%r, charge=%r, initial=%r)" % (
            self._regions,
            self._d,
            self._name,
            self._charge,
            self.initial,
        )

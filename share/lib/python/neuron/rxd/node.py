import neuron
from neuron import h, nrn, hoc, nrn_dll_sym
from . import region, constants
from . import rxdsection
import numpy
import weakref
from .rxdException import RxDException
import warnings
import ctypes

from collections.abc import Callable


# function to change extracellular diffusion
set_diffusion = nrn_dll_sym("set_diffusion")
set_diffusion.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    numpy.ctypeslib.ndpointer(ctypes.c_double),
    ctypes.c_int,
]
set_diffusion.restype = ctypes.c_int

# data storage
_volumes = numpy.array([])
_surface_area = numpy.array([])
_diffs = numpy.array([])
_states = numpy.array([])
_node_fluxes = {"index": [], "type": [], "source": [], "scale": [], "region": []}
_has_node_fluxes = False

_point_indices = {}

# node data types
_concentration_node = 0
_molecule_node = 1


def _get_data():
    return (_volumes, _surface_area, _diffs)


def _get_states():
    return _states


def _allocate(num):
    """allocate storage for num more nodes, return the starting index

    Note: no guarantee is made of preserving previous _ref
    """
    global _volumes, _surface_area, _diffs, _states
    start_index = len(_volumes)
    total = start_index + num
    _volumes.resize(total, refcheck=False)
    _surface_area.resize(total, refcheck=False)
    _diffs.resize(total, refcheck=False)
    _states.resize(total, refcheck=False)
    return start_index


def _remove(start, stop):
    """delete old volumes, surface areas and diff values in from global arrays"""
    global _volumes, _surface_area, _diffs, _states, _node_fluxes, _has_node_fluxes
    # Remove entries that have to be recalculated
    dels = list(range(start, stop))
    _volumes = numpy.delete(_volumes, dels)
    _surface_area = numpy.delete(_surface_area, dels)
    _diffs = numpy.delete(_diffs, dels)
    _states = numpy.delete(_states, dels)

    # remove _node_flux
    newflux = {"index": [], "type": [], "source": [], "scale": [], "region": []}
    for (i, idx) in enumerate(_node_fluxes["index"]):
        if idx not in dels:
            for key in _node_fluxes:
                newflux[key].append(_node_fluxes[key][i])
    _node_fluxes = newflux
    _has_node_fluxes = _node_fluxes["index"] != []

    # remove _node_flux
    for (i, idx) in enumerate(_node_fluxes["index"]):
        if idx in dels:
            for lst in _node_fluxes.values():
                del lst[i]


def _replace(old_offset, old_nseg, new_offset, new_nseg):
    """delete old volumes, surface areas and diff values in from global arrays
    move states so that the new segment value is equal to the old segment
    value that contains its centre"""
    global _volumes, _surface_area, _diffs, _states
    # remove entries that have to be recalculated
    start = old_offset
    stop = start + old_nseg + 1

    dels = list(range(start, stop))
    _volumes = numpy.delete(_volumes, dels)
    _surface_area = numpy.delete(_surface_area, dels)
    _diffs = numpy.delete(_diffs, dels)

    # replace states -- the new segment has the state from the old
    # segment which contains it's centre

    for j in range(new_nseg):
        i = int(((j + 0.5) / new_nseg) * old_nseg)
        _states[new_offset + j] = _states[old_offset + i]
    _states = numpy.delete(_states, list(range(start, stop)))

    # update _node_flux index
    for (i, idx) in enumerate(_node_fluxes["index"]):
        if idx in dels:
            j = int(((idx + 0.5) / new_nseg) * old_nseg)
            _node_fluxes["index"][i] = j


_numpy_element_ref = neuron.numpy_element_ref


class Node(object):
    def satisfies(self, condition):
        """Tests if a Node satisfies a given condition.

        If a nrn.Section object or RxDSection is provided, returns True if the Node lies in the section; else False.
        If a Region object is provided, returns True if the Node lies in the Region; else False.
        If a number between 0 and 1 is provided, returns True if the normalized position lies within the Node; else False.
        """
        if isinstance(condition, nrn.Section) or isinstance(
            condition, rxdsection.RxDSection
        ):
            return self._in_sec(condition)
        elif isinstance(condition, region.Region):
            return self.region == condition
        elif isinstance(condition, nrn.Segment):
            return self.segment == condition
        elif isinstance(condition, region.Extracellular):
            return self.region == condition
        raise RxDException("selector %r not supported for this node type" % condition)

    @property
    def _ref_concentration(self):
        """Returns a NEURON reference to the Node's concentration

        (The node must store concentration data. Use _ref_molecules for nodes
        storing molecule counts.)
        """
        # this points to rxd array only, will not change legacy concentration
        if self._data_type == _concentration_node:
            return self._ref_value
        else:
            raise RxDException(
                "_ref_concentration only available for concentration nodes"
            )

    @property
    def _ref_molecules(self):
        """Returns a NEURON reference to the Node's concentration

        (The node must store molecule counts. Use _ref_concentrations for nodes
        storing concentration.)
        """
        # this points to rxd array only, will not change legacy concentration
        if self._data_type == _molecule_node:
            return self._ref_value
        else:
            raise RxDException("_ref_molecules only available for molecule count nodes")

    @property
    def d(self):
        """Gets the diffusion rate within the compartment."""
        return _diffs[self._index]

    @d.setter
    def d(self, value):
        """Sets the diffusion rate within the compartment."""
        from . import rxd

        # TODO: make invalidation work so don't need to redo the setup each time
        # rxd._invalidate_matrices()
        _diffs[self._index] = value
        rxd._setup_matrices()

    @property
    def concentration(self):
        """Gets the concentration at the Node."""
        # TODO: don't use an if statement here... put the if statement at node
        #       construction and change the actual function that is pointed to
        if self._data_type == _concentration_node:
            return self.value
        else:
            # TODO: make this return a concentration instead of an error
            raise RxDException(
                "concentration property not yet supported for non-concentration nodes"
            )

    @concentration.setter
    def concentration(self, value):
        """Sets the concentration at the Node"""
        # TODO: don't use an if statement here... put the if statement at node
        #       construction and change the actual function that is pointed to
        if self._data_type == _concentration_node:
            self.value = value
        else:
            # TODO: make this set a concentration instead of raise an error
            raise RxDException(
                "concentration property not yet supported for non-concentration nodes"
            )

    @property
    def molecules(self):
        """Gets the molecule count at the Node."""
        # TODO: don't use an if statement here... put the if statement at node
        #       construction and change the actual function that is pointed to
        if self._data_type == _molecule_node:
            return self.value
        else:
            # TODO: make this return a molecule count instead of an error
            raise RxDException(
                "molecules property not yet supported for non-concentration nodes"
            )

    @molecules.setter
    def molecules(self, value):
        """Sets the molecule count at the Node"""
        # TODO: don't use an if statement here... put the if statement at node
        #       construction and change the actual function that is pointed to
        if self._data_type == _molecule_node:
            self.value = value
        else:
            # TODO: make this set a molecule count instead of raise an error
            raise RxDException(
                "molecules property not yet supported for non-concentration nodes"
            )

    @property
    def value(self):
        """Gets the value associated with this Node."""
        return _states[self._index]

    @value.setter
    def value(self, v):
        """Sets the value associated with this Node.

        For Species nodes belonging to a deterministic simulation, this is a concentration.
        For Species nodes belonging to a stochastic simulation, this is the molecule count.
        """
        _states[self._index] = v

    @property
    def _ref_value(self):
        """Returns a NEURON reference to the Node's value"""
        return _numpy_element_ref(_states, self._index)

    def include_flux(self, *args, **kwargs):
        """Include a flux contribution to a specific node.

        The flux can be described as a NEURON reference, a point process and a
        property, a Python function, or something that evaluates to a constant
        Python float.

        Supported units:
            molecule/ms
            mol/ms
            mmol/ms == millimol/ms == mol/s

        Examples:
            node.include_flux(mglur, 'ip3flux')           # default units: molecule/ms
            node.include_flux(mglur, 'ip3flux', units='mol/ms') # units: moles/ms
            node.include_flux(mglur._ref_ip3flux, units='molecule/ms')
            node.include_flux(lambda: mglur.ip3flux)
            node.include_flux(lambda: math.sin(h.t))
            node.include_flux(47)

        Warning:
            Flux denotes a change in *mass* not a change in concentration.
            For example, a metabotropic synapse produces a certain amount of
            substance when activated. The corresponding effect on the node's
            concentration depends on the volume of the node. (This scaling is
            handled automatically by NEURON's rxd module.)
        """
        global _has_node_fluxes, _node_fluxes
        if len(args) not in (1, 2):
            raise RxDException("node.include_flux takes only one or two arguments")
        if "units" in kwargs:
            units = kwargs.pop("units")
        else:
            units = "molecule/ms"
        if len(kwargs):
            raise RxDException("Unknown keyword arguments: %r" % list(kwargs.keys()))
        # take the value, divide by scale to get mM um^3
        # once this is done, we need to divide by volume to get mM
        # TODO: is division still slower than multiplication? Switch to mult.
        if units == "molecule/ms":
            scale = constants.molecules_per_mM_um3()
        elif units == "mol/ms":
            # You have: mol
            # You want: (millimol/L) * um^3
            #    * 1e+18
            #    / 1e-18
            scale = 1e-18
        elif units in ("mmol/ms", "millimol/ms", "mol/s"):
            # You have: millimol
            # You want: (millimol/L)*um^3
            #    * 1e+15
            #    / 1e-15
            scale = 1e-15
        else:
            raise RxDException("unknown unit: %r" % units)

        if len(args) == 1 and isinstance(args[0], hoc.HocObject):
            source = args[0]
            flux_type = 1
            try:
                # just a test access
                source[0]
            except:
                raise RxDException("HocObject must be a pointer")
        elif len(args) == 1 and isinstance(args[0], Callable):
            flux_type = 2
            source = args[0]
            warnings.warn(
                "Adding a python callback may slow down execution. Consider using a Rate and Parameter."
            )
        elif len(args) == 2:
            flux_type = 1
            try:
                source = getattr(args[0], "_ref_" + args[1])
            except:
                raise RxDException("Invalid two parameter form")

            # TODO: figure out a units checking solution that works
            # source_units = h.units(source)
            # if source_units and source_units != units:
            #    warnings.warn('Possible units conflict. NEURON says %r, but specified as %r.' % (source_units, units))
        else:
            success = False
            if len(args) == 1:
                try:
                    f = float(args[0])
                    source = f
                    flux_type = 3
                    success = True
                except:
                    pass
            if not success:
                raise RxDException("unsupported flux form")
        _node_fluxes["index"].append(self._index)
        if isinstance(self, Node1D):
            _node_fluxes["type"].append(-1)
        else:
            _node_fluxes["type"].append(self._grid_id)
        _node_fluxes["source"].append(source)
        _node_fluxes["scale"].append(scale)
        if isinstance(self, Node1D):
            _node_fluxes["region"].append(None)
        else:
            _node_fluxes["region"].append(weakref.ref(self._r))
        from .rxd import _structure_change_count

        _structure_change_count.value += 1
        _has_node_fluxes = True

    @value.getter
    def _state_index(self):
        return self._index


class Node1D(Node):
    def __init__(self, sec, i, location, data_type=_concentration_node):
        """n = Node1D(sec, i, location)
        Description:

        Constructs a Node object. These encapsulate multiple properties of a given
        reaction-diffusion compartment -- volume, surface area, concentration, etc... --
        but this association is provided only as a convenience to the user; all the data
        exists independently of the Node object.

        These objects are typically constructed by the reaction-diffusion code and
        provided to the user.

        Parameters:

        sec  -- the RxDSection containing the Node

        i -- the offset into the RxDSection's data

        location -- the location of the compartment within the section. For @aSection1D objects, this is the normalized position 0 <= x <= 1
        """
        self._sec = sec
        self._location = location
        self._index = i + sec._offset
        self._loc3d = None
        self._data_type = data_type

    def _update_loc3d(self):
        sec = self._sec
        length = sec.L
        normalized_arc3d = [sec._sec.arc3d(i) / length for i in range(sec._sec.n3d())]
        x3d = [sec._sec.x3d(i) for i in range(sec._sec.n3d())]
        y3d = [sec._sec.y3d(i) for i in range(sec._sec.n3d())]
        z3d = [sec._sec.z3d(i) for i in range(sec._sec.n3d())]
        loc1d = self._location
        self._loc3d = (
            numpy.interp(loc1d, normalized_arc3d, x3d),
            numpy.interp(loc1d, normalized_arc3d, y3d),
            numpy.interp(loc1d, normalized_arc3d, z3d),
        )

    def satisfies(self, condition):
        """Tests if a Node satisfies a given condition.

        If a nrn.Section object or RxDSection is provided, returns True if the Node lies in the section; else False.
        If a Region object is provided, returns True if the Node lies in the Region; else False.
        If a number between 0 and 1 is provided, returns True if the normalized position lies within the Node; else False.
        """
        try:
            # test Node conditions
            return super(Node1D, self).satisfies(condition)
        except:
            pass  # ignore the error and try Node1D specific conditions
        try:
            if 0 <= condition <= 1:
                dx = 1.0 / self._sec.nseg
                check_index = int(condition * self._sec.nseg)
                if check_index >= self._sec.nseg:
                    # if here, then at the 1 end
                    check_index = self._sec.nseg - 1
                # self_index should be unique (no repeats due to roundoff error) because the inside should always be 0.5 over an integer
                self_index = int(self._location * self._sec.nseg)
                return check_index == self_index

        except:
            pass
        raise RxDException("unrecognized node condition: %r" % condition)

    @property
    def x3d(self):
        """x coordinate"""
        if self._loc3d is None:
            self._update_loc3d()
        return self._loc3d[0]

    @property
    def y3d(self):
        """y coordinate"""
        if self._loc3d is None:
            self._update_loc3d()
        return self._loc3d[1]

    @property
    def z3d(self):
        """z coordinate"""
        if self._loc3d is None:
            self._update_loc3d()
        return self._loc3d[2]

    @property
    def volume(self):
        """The volume of the compartment in cubic microns.

        Read only."""
        from . import rxd

        rxd._update_node_data()
        return _volumes[self._index]

    @property
    def segment(self):
        return self._sec._sec(self.x)

    @property
    def surface_area(self):
        """The surface area of the compartment in square microns.

        This is the area (if any) of the compartment that lies on the plasma membrane
        and therefore is the area used to determine the contribution of currents (e.g. ina) from
        mod files or kschan to the compartment's concentration.

        Read only.
        """
        from . import rxd

        rxd._update_node_data()
        return _surface_area[self._index]

    @property
    def x(self):
        """The normalized position of the center of the compartment.

        Read only."""
        # TODO: will probably want to change this to be more generic for higher dimensions
        return self._location

    @property
    def region(self):
        """The region containing the compartment."""
        return self._sec._region

    @property
    def sec(self):
        """The Section containing the compartment."""
        return self._sec._sec

    def _in_sec(self, sec):
        return sec == self.sec or sec == self._sec

    @property
    def species(self):
        """The Species whose concentration is recorded at this Node."""
        return self._sec._species()


class Node3D(Node):
    def __init__(
        self, index, i, j, k, r, d, seg, speciesref, data_type=_concentration_node
    ):
        """
        Parameters
        ----------

        index : int
            the offset into the global rxd data
        i : int
            the x coordinate in the region's matrix
        j : int
            the y coordinate in the region's matrix
        k : int
            the z coordinate in the region's matrix
        r : rxd.Region
            the region that contains this node
        seg : nrn.Segment
            the segment containing this node
        """
        self._index = index
        self._i = i
        self._j = j
        self._k = k
        self._neighbors = None
        # TODO: store region as a weakref! (weakref.proxy?)
        self._r = r
        self._sec = h.SectionList([seg.sec])
        self._x = seg.x
        self._speciesref = speciesref
        self._data_type = data_type

        _point_indices.setdefault(self._r, {})
        _point_indices[self._r][(self._i, self._j, self._k)] = self._index

    @property
    def _seg(self):
        return list(self._sec)[0](self._x) if list(self._sec) else None

    def _find_neighbors(self):
        self._pos_x_neighbor = _point_indices[self._r].get(
            (self._i + 1, self._j, self._k)
        )
        self._neg_x_neighbor = _point_indices[self._r].get(
            (self._i - 1, self._j, self._k)
        )
        self._pos_y_neighbor = _point_indices[self._r].get(
            (self._i, self._j + 1, self._k)
        )
        self._neg_y_neighbor = _point_indices[self._r].get(
            (self._i, self._j - 1, self._k)
        )
        self._pos_z_neighbor = _point_indices[self._r].get(
            (self._i, self._j, self._k + 1)
        )
        self._neg_z_neighbor = _point_indices[self._r].get(
            (self._i, self._j, self._k - 1)
        )

        self._neighbors = (
            self._pos_x_neighbor,
            self._neg_x_neighbor,
            self._pos_y_neighbor,
            self._neg_y_neighbor,
            self._pos_z_neighbor,
            self._neg_z_neighbor,
        )

        return self._neighbors

    @property
    def neighbors(self):
        return (
            self._neighbors if self._neighbors is not None else self._find_neighbors()
        )

    @property
    def _grid_id(self):
        return self._speciesref()._intracellular_instances[self._r]._grid_id

    @property
    def surface_area(self):
        """The surface area of the compartment in square microns.

        This is the area (if any) of the compartment that lies on the plasma membrane
        and therefore is the area used to determine the contribution of currents (e.g. ina) from
        mod files or kschan to the compartment's concentration.

        Read only.
        """
        # TODO: should I have the commented out line?
        # rxd._update_node_data()
        return self._r._sa[self._index]

    def satisfies(self, condition):
        """Tests if a Node satisfies a given condition.

        If a nrn.Section object or RxDSection is provided, returns True if the Node lies in the section; else False.
        If a Region object is provided, returns True if the Node lies in the Region; else False.
        If a tuple is provided of length 3, return True if the Node contains the (x, y, z) point; else False.

        Does not currently support numbers between 0 and 1.
        """
        try:
            # test the Node conditions
            return super(Node3D, self).satisfies(condition)
        except:
            pass  # ignore the error and try Node3D specific conditions

        if isinstance(condition, tuple) and len(condition) == 3:
            x, y, z = condition
            mesh = self._r._mesh_grid
            return (
                int((x - mesh["xlo"]) / mesh["dx"]) == self._i
                and int((y - mesh["ylo"]) / mesh["dy"]) == self._j
                and int((z - mesh["zlo"]) / mesh["dz"]) == self._k
            )
        # check for a position condition so as to provide a more useful error
        try:
            if 0 <= condition <= 1:
                # TODO: the trouble here is that you can't do this super-directly based on x
                #       the way to do this is to find the minimum and maximum x values contained in the grid
                #       the extra difficulty with that is to handle boundary cases correctly
                #       (to test, consider a section 1 node wide by 15 discretized pieces long, access at 1./15, 2./15, etc...)
                raise RxDException(
                    "selecting nodes by normalized position not yet supported for 3D nodes; see comments in source about how to fix this"
                )
        except:
            pass
        raise RxDException("unrecognized node condition: %r" % condition)

    @property
    def x3d(self):
        # TODO: need to modify this to work with 1d
        mesh = self._r._mesh_grid
        return mesh["xlo"] + (self._i + 0.5) * mesh["dx"]

    @property
    def y3d(self):
        # TODO: need to modify this to work with 1d
        mesh = self._r._mesh_grid
        return mesh["ylo"] + (self._j + 0.5) * mesh["dy"]

    @property
    def z3d(self):
        # TODO: need to modify this to work with 1d
        mesh = self._r._mesh_grid
        return mesh["zlo"] + (self._k + 0.5) * mesh["dz"]

    @property
    def x(self):
        # TODO: can we make this more accurate?
        return self._x

    @property
    def segment(self):
        return self._seg

    def _in_sec(self, sec):
        return sec == self.sec

    @property
    def sec(self):
        return list(self._sec)[0] if any(self._sec) else None

    @property
    def volume(self):
        return self._r._vol[self._index]

    @property
    def region(self):
        """The region containing the compartment."""
        return self._r

    @property
    def species(self):
        """The Species whose concentration is recorded at this Node."""
        return self._speciesref()

    @property
    def d(self):
        """Gets the value associated with this Node."""
        sp = self._speciesref()._intracellular_instances[self._r]
        if hasattr(sp, "_dgrid"):
            return sp._dgrid[self._index]
        return sp._d

    @d.setter
    def d(self, v):
        sp = self._speciesref()._intracellular_instances[self._r]
        if not hasattr(sp, "_dgrid"):
            sp._dgrid = numpy.ndarray((3, sp._nodes_length), dtype=float, order="C")
            sp._dgrid[:] = sp._d.reshape(3, -1)
            set_diffusion(0, self._grid_id, sp._dgrid, sp._nodes_length)
        sp._dgrid[:, self._index] = v

    @property
    def value(self):
        """Gets the value associated with this Node."""
        return self._speciesref()._intracellular_instances[self._r].states[self._index]

    @value.setter
    def value(self, v):
        """Sets the value associated with this Node.

        For Species nodes belonging to a deterministic simulation, this is a concentration.
        For Species nodes belonging to a stochastic simulation, this is the molecule count.
        """
        self._speciesref()._intracellular_instances[self._r].states[self._index] = v

    @property
    def _ref_value(self):
        """Returns a HOC reference to the Node's value"""
        return (
            self._speciesref()
            ._intracellular_instances[self._r]
            ._states._ref_x[self._index]
        )


class NodeExtracellular(Node):
    def __init__(self, index, i, j, k, speciesref, regionref):
        """
        Parameters
        ----------

        index : int
            the offset into the global rxd data
        i : int
            the x coordinate in the region's matrix
        j : int
            the y coordinate in the region's matrix
        k : int
            the z coordinate in the region's matrix
        """
        self._index = index
        self._i = i
        self._j = j
        self._k = k
        # TODO: store region as a weakref! (weakref.proxy?)
        self._speciesref = speciesref
        self._regionref = regionref
        # TODO: support _molecule_node data type
        self._data_type = _concentration_node

    @property
    def x3d(self):
        return self._regionref()._xlo + (self._i + 0.5) * self._regionref()._dx[0]

    @property
    def y3d(self):
        return self._regionref()._ylo + (self._j + 0.5) * self._regionref()._dx[1]

    @property
    def z3d(self):
        return self._regionref()._zlo + (self._k + 0.5) * self._regionref()._dx[2]

    @property
    def region(self):
        """The extracellular space containing the node."""
        return self._regionref()

    @property
    def _r(self):
        """The extracellular space containing the node."""
        return self._regionref()

    @property
    def d(self):
        """Gets the diffusion rate within the compartment."""
        return self._speciesref()._d

    @d.setter
    def d(self, value):
        """Sets the diffusion rate within the compartment."""
        from . import rxd

        warnings.warn(
            "Changing the diffusion coefficient for an extracellular node changes diffusion coefficients for the whole extracellular grid."
        )
        # TODO: Replace zero with Parallel_grids id (here an in insert call)
        if hasattr(value, "__len__"):
            set_diffusion(0, self._grid_id, numpy.array(value, dtype=float), 1)
        else:
            set_diffusion(0, self._grid_id, numpy.repeat(float(value), 3), 1)

    @property
    def value(self):
        """Gets the value associated with this Node."""
        return (
            self._speciesref().__getitem__(self._r).states3d[self._i, self._j, self._k]
        )

    @value.setter
    def value(self, v):
        """Sets the value associated with this Node.

        For Species nodes belonging to a deterministic simulation, this is a concentration.
        For Species nodes belonging to a stochastic simulation, this is the molecule count.
        """
        self._speciesref().__getitem__(self._r).states3d[self._i, self._j, self._k] = v

    @property
    def _ref_value(self):
        """Returns a HOC reference to the Node's value"""
        return (
            self._speciesref()
            .__getitem__(self._regionref())
            ._extracellular()
            ._states._ref_x[self._index]
        )

    @value.getter
    def _state_index(self):
        return self._index

    @property
    def volume(self):
        ecs = self._regionref()
        if numpy.isscalar(ecs.alpha):
            return numpy.prod(ecs._dx) * ecs.alpha
        else:
            return numpy.prod(ecs._dx) * ecs.alpha[self._i, self._j, self._k]

    @property
    def _grid_id(self):
        return self._speciesref()._extracellular_instances[self._r]._grid_id

    def satisfies(self, condition):
        """Tests if a Node satisfies a given condition.

        If a nrn.Section object or RxDSection is provided, returns True if the Node lies in the section; else False.
        If a Region object is provided, returns True if the Node lies in the Region; else False.
        If a tuple is provided of length 3, return True if the Node contains the (x, y, z) point; else False.
        """
        try:
            # test the Node conditions
            return super(NodeExtracellular, self).satisfies(condition)
        except:
            pass  # ignore the error and try NodeExtracellular specific conditions

        if isinstance(condition, tuple) and len(condition) == 3:
            x, y, z = condition
            r = self._regionref()
            return (
                int((x - r._xlo) / r._dx[0]) == self._i
                and int((y - r._ylo) / r._dx[1]) == self._j
                and int((z - r._zlo) / r._dx[2]) == self._k
            )
        raise RxDException("unrecognized node condition: %r" % condition)

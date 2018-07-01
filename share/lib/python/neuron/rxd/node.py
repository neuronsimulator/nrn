import neuron
from neuron import h, nrn, hoc
from . import region
from . import rxdsection
import numpy
import weakref
from .rxdException import RxDException
import warnings
import collections

# data storage
_volumes = numpy.array([])
_surface_area = numpy.array([])
_diffs = numpy.array([])
_states = numpy.array([])
_node_fluxes = {'indices': [], 'type': [], 'source': [], 'scale': []}
_has_node_fluxes = False

# node data types
_concentration_node = 0
_molecule_node = 1

def _apply_node_fluxes(dy):
    # TODO: what if the nodes go away because a section is destroyed?
    if _has_node_fluxes:
        # TODO: be smarter. Use PtrVector when possible. Handle constants efficiently.
        for index, t, source, scale in zip(_node_fluxes['indices'], _node_fluxes['type'], _node_fluxes['source'], _node_fluxes['scale']):
            if t == 1:
                delta = source[0]
            elif t == 2:
                delta = source()
            elif t == 3:
                delta = source
            else:
                raise RxDException('Unknown flux source type. Users should never see this.')
            # TODO: check this... make sure scale shouldn't be divided by volume
            dy[index] += delta / scale / _volumes[index]

def _get_data():
    return (_volumes, _surface_area, _diffs)

def _get_states():
    return _states

def _allocate(num):
    """allocate storage for num more nodes, return the starting index
    
    Note: no guarantee is made of preserving previous _ref
    """
    start_index = len(_volumes)
    total = start_index + num
    _volumes.resize(total, refcheck=False)
    _surface_area.resize(total, refcheck=False)
    _diffs.resize(total, refcheck=False)
    _states.resize(total, refcheck=False)
    return start_index

_numpy_element_ref = neuron.numpy_element_ref

class Node(object):
    def satisfies(self, condition):
        """Tests if a Node satisfies a given condition.

        If a nrn.Section object or RxDSection is provided, returns True if the Node lies in the section; else False.
        If a Region object is provided, returns True if the Node lies in the Region; else False.
        If a number between 0 and 1 is provided, returns True if the normalized position lies within the Node; else False.
        """
        if isinstance(condition, nrn.Section) or isinstance(condition, rxdsection.RxDSection):
            return self._in_sec(condition)
        elif isinstance(condition, region.Region):
            return self.region == condition
        elif isinstance(condition, nrn.Segment):
            return self.segment == condition
        raise RxDException('selector %r not supported for this node type' % condition)
        
    @property
    def _ref_concentration(self):
        """Returns a HOC reference to the Node's concentration
        
        (The node must store concentration data. Use _ref_molecules for nodes
        storing molecule counts.)
        """
        # this points to rxd array only, will not change legacy concentration
        if self._data_type == _concentration_node:        
            return self._ref_value
        else:
            raise RxDException('_ref_concentration only available for concentration nodes')

    @property
    def _ref_molecules(self):
        """Returns a HOC reference to the Node's concentration
        
        (The node must store concentration data. Use _ref_molecules for nodes
        storing molecule counts.)
        """
        # this points to rxd array only, will not change legacy concentration
        if self._data_type == _molecule_node:        
            return self._ref_value
        else:
            raise RxDException('_ref_molecules only available for molecule count nodes')
    
    @property
    def d(self):
        """Gets the diffusion rate within the compartment."""
        return _diffs[self._index]
    @d.setter
    def d(self, value):
        """Sets the diffusion rate within the compartment."""
        from . import rxd
        # TODO: make invalidation work so don't need to redo the setup each time
        #rxd._invalidate_matrices()
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
            raise RxDException('concentration property not yet supported for non-concentration nodes')
    
    @concentration.setter
    def concentration(self, value):
        """Sets the concentration at the Node"""
        # TODO: don't use an if statement here... put the if statement at node
        #       construction and change the actual function that is pointed to
        if self._data_type == _concentration_node:
            self.value = value
        else:
            # TODO: make this set a concentration instead of raise an error
            raise RxDException('concentration property not yet supported for non-concentration nodes')

    @property
    def molecules(self):
        """Gets the molecule count at the Node."""
        # TODO: don't use an if statement here... put the if statement at node
        #       construction and change the actual function that is pointed to
        if self._data_type == _molecule_node:
            return self.value
        else:
            # TODO: make this return a molecule count instead of an error
            raise RxDException('molecules property not yet supported for non-concentration nodes')
    
    @molecules.setter
    def molecules(self, value):
        """Sets the molecule count at the Node"""
        # TODO: don't use an if statement here... put the if statement at node
        #       construction and change the actual function that is pointed to
        if self._data_type == _molecule_node:
            self.value = value
        else:
            # TODO: make this set a molecule count instead of raise an error
            raise RxDException('molecules property not yet supported for non-concentration nodes')
        

        
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
        """Returns a HOC reference to the Node's value"""
        return _numpy_element_ref(_states, self._index)
    
    def include_flux(self, *args, **kwargs):
        """Include a flux contribution to a specific node.
        
        The flux can be described as a HOC reference, a point process and a
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
        global _has_node_fluxes
        if len(args) not in (1, 2):
            raise RxDException('node.include_flux takes only one or two arguments')
        if 'units' in kwargs:
            units = kwargs.pop('units')
        else:
            units = 'molecule/ms'
        if len(kwargs):
            raise RxDException('Unknown keyword arguments: %r' % list(kwargs.keys()))
        # take the value, divide by scale to get mM um^3
        # once this is done, we need to divide by volume to get mM
        # TODO: is division still slower than multiplication? Switch to mult.
        if units == 'molecule/ms':
            scale = 602214.129
        elif units == 'mol/ms':
            # You have: mol
            # You want: (millimol/L) * um^3
            #    * 1e+18
            #    / 1e-18
            scale = 1e-18
        elif units in ('mmol/ms', 'millimol/ms', 'mol/s'):
            # You have: millimol
            # You want: (millimol/L)*um^3
            #    * 1e+15
            #    / 1e-15
            scale = 1e-15
        else:
            raise RxDException('unknown unit: %r' % units)
        
        if len(args) == 1 and isinstance(args[0], hoc.HocObject):
            source = args[0]
            flux_type = 1
            try:
                # just a test access
                source[0]
            except:
                raise RxDException('HocObject must be a pointer')
        elif len(args) == 1 and isinstance(args[0], collections.Callable):
            flux_type = 2
            source = args[0]
        elif len(args) == 2:
            flux_type = 1
            try:
                source = args[0].__getattribute__('_ref_' + args[1])
            except:
                raise RxDException('Invalid two parameter form')
            
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
                raise RxDException('unsupported flux form')
        
        _node_fluxes['indices'].append(self._index)
        _node_fluxes['type'].append(flux_type)
        _node_fluxes['source'].append(source)
        _node_fluxes['scale'].append(scale)
        _has_node_fluxes = True
        
    
_h_n3d = h.n3d
_h_x3d = h.x3d
_h_y3d = h.y3d
_h_z3d = h.z3d
_h_arc3d = h.arc3d

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
        normalized_arc3d = [_h_arc3d(i, sec=sec._sec) / length for i in range(int(_h_n3d(sec=sec._sec)))]
        x3d = [_h_x3d(i, sec=sec._sec) for i in range(int(_h_n3d(sec=sec._sec)))]
        y3d = [_h_y3d(i, sec=sec._sec) for i in range(int(_h_n3d(sec=sec._sec)))]
        z3d = [_h_z3d(i, sec=sec._sec) for i in range(int(_h_n3d(sec=sec._sec)))]
        loc1d = self._location
        self._loc3d = (numpy.interp(loc1d, normalized_arc3d, x3d),
                       numpy.interp(loc1d, normalized_arc3d, y3d),
                       numpy.interp(loc1d, normalized_arc3d, z3d))
        
    def satisfies(self, condition):
        """Tests if a Node satisfies a given condition.

        If a nrn.Section object or RxDSection is provided, returns True if the Node lies in the section; else False.
        If a Region object is provided, returns True if the Node lies in the Region; else False.
        If a number between 0 and 1 is provided, returns True if the normalized position lies within the Node; else False.
        """
        if isinstance(condition, nrn.Section) or isinstance(condition, rxdsection.RxDSection):
            return self._in_sec(condition)
        elif isinstance(condition, region.Region):
            return self.region == condition
        elif isinstance(condition, nrn.Segment):
            return self.segment == condition
        try:
            if 0 <= condition <= 1:
                dx = 1. / self._sec.nseg
                check_index = int(condition * self._sec.nseg)
                if check_index >= self._sec.nseg:
                    # if here, then at the 1 end
                    check_index = self._sec.nseg - 1
                # self_index should be unique (no repeats due to roundoff error) because the inside should always be 0.5 over an integer
                self_index = int(self._location * self._sec.nseg)
                return check_index == self_index
                
        except:
            raise RxDException('unrecognized node condition: %r' % condition)
            
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
        """The RxDSection containing the compartment."""
        return self._sec

    def _in_sec(self, sec):
        return sec == self.sec or sec == self.sec._sec

    
    @property
    def species(self):
        """The Species whose concentration is recorded at this Node."""
        return self._sec._species()    


class Node3D(Node):
    def __init__(self, index, i, j, k, r, seg, speciesref, data_type=_concentration_node):
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
        # TODO: store region as a weakref! (weakref.proxy?)
        self._r = r
        self._seg = seg
        self._speciesref = speciesref
        self._data_type = data_type
    
    @property
    def surface_area(self):
        """The surface area of the compartment in square microns.
        
        This is the area (if any) of the compartment that lies on the plasma membrane
        and therefore is the area used to determine the contribution of currents (e.g. ina) from
        mod files or kschan to the compartment's concentration.
        
        Read only.
        """
        # TODO: should I have the commented out line?
        #rxd._update_node_data()
        return _surface_area[self._index]
        
    def satisfies(self, condition):
        """Tests if a Node satisfies a given condition.

        If a nrn.Section object or RxDSection is provided, returns True if the Node lies in the section; else False.
        If a Region object is provided, returns True if the Node lies in the Region; else False.
        If a number between 0 and 1 is provided, returns True if the normalized position lies within the Node; else False.
        """
        if isinstance(condition, nrn.Section) or isinstance(condition, rxdsection.RxDSection):
            return self._in_sec(condition)
        elif isinstance(condition, region.Region):
            return self.region == condition
        elif isinstance(condition, nrn.Segment):
            return self.segment == condition
        position_type = False
        try:
            if 0 <= condition <= 1:
                position_type = True
        except:
            raise RxDException('unrecognized node condition: %r' % condition)    
        if position_type:
            # TODO: the trouble here is that you can't do this super-directly based on x
            #       the way to do this is to find the minimum and maximum x values contained in the grid
            #       the extra difficulty with that is to handle boundary cases correctly
            #       (to test, consider a section 1 node wide by 15 discretized pieces long, access at 1./15, 2./15, etc...)
            raise RxDException('selecting nodes by normalized position not yet supported for 3D nodes; see comments in source about how to fix this')
    @property
    def x3d(self):
        # TODO: need to modify this to work with 1d
        return self._r._mesh.xs[self._i]
    @property
    def y3d(self):
        # TODO: need to modify this to work with 1d
        return self._r._mesh.ys[self._j]
    @property
    def z3d(self):
        # TODO: need to modify this to work with 1d
        return self._r._mesh.zs[self._k]
    
    @property
    def x(self):
        # TODO: can we make this more accurate?
        return self._seg.x
    
    @property
    def segment(self):
        return self._seg
    
    def _in_sec(self, sec):
        return sec == self.sec
    
    @property
    def sec(self):
        if self._seg is None:
            return None
        return self._seg.sec
    
    @property
    def volume(self):
        return _volumes[self._index]

    @property
    def region(self):
        """The region containing the compartment."""
        return self._r

    @property
    def species(self):
        """The Species whose concentration is recorded at this Node."""
        return self._speciesref()
    
    @property
    def value(self):
        """Gets the value associated with this Node."""
        from . import rxd
        if rxd._external_solver is not None:
            _states[self._index] = rxd._external_solver.value(self)
        return _states[self._index]
    
    @value.setter
    def value(self, v):
        """Sets the value associated with this Node.
        
        For Species nodes belonging to a deterministic simulation, this is a concentration.
        For Species nodes belonging to a stochastic simulation, this is the molecule count.
        """
        from . import rxd
        _states[self._index] = v
        if rxd._external_solver is not None:
            rxd._external_solver.update_value(self)

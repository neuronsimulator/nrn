import neuron
from neuron import h, nrn
import rxd
import numpy
import weakref
import region
import rxdsection

# data storage
_volumes = numpy.array([])
_surface_area = numpy.array([])
_diffs = numpy.array([])
_states = numpy.array([])

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
        try:
            dx = 1. / self.sec.nseg / 2.
            if 0 < condition <= 1:
                return -dx < self.x - condition <= dx
            elif condition == 0:
                # nodes at dx, 3dx, 5dx, 7dx, etc... so this allows for roundoff errors
                return self.x < 2. * dx
                
        except:
            raise Exception('unrecognized node condition: %r' % condition)

class Node1D(Node):
    def __init__(self, sec, i, location):
        """n = Node(sec, i, location)
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
    
    @property
    def _ref_concentration(self):
        """Returns a HOC reference to the Node's state"""
        # this points to rxd array only, will not change legacy concentration
        return neuron.numpy_element_ref(_states, self._index)



    @property
    def volume(self):
        """The volume of the compartment in cubic microns.
        
        Read only."""
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
        
        rxd._update_node_data()
        return _surface_area[self._index]
    
        
    @property
    def x(self):
        """The normalized position of the center of the compartment.
        
        Read only."""
        # TODO: will probably want to change this to be more generic for higher dimensions
        return self._location
    
    @property
    def d(self):
        """Gets the diffusion rate within the compartment."""
        return _diffs[self._index]
    @d.setter
    def d(self, value):
        """Sets the diffusion rate within the compartment."""
        # TODO: make invalidation work so don't need to redo the setup each time
        #rxd._invalidate_matrices()
        _diffs[self._index] = value
        rxd._setup_matrices()
    
    @property
    def region(self):
        """The region containing the compartment."""
        return self._sec.region()
    
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
    
    @property
    def value(self):
        """Gets the value associated with this Node."""
        # TODO: change if stochastic allows molecules
        return self.concentration
    
    @value.setter
    def value(self, v):
        """Sets the value associated with this Node."""
        # TODO: change if stochastic allows molecules
        self.concentration = v


    @property
    def _ref_value(self):
        """Returns a HOC reference to the Node's value"""
        # TODO: change this if value no longer need be concentration
        return self._ref_concentration
    
    @property
    def concentration(self):
        """Gets the concentration at the Node."""
        return _states[self._index]
        
    @concentration.setter
    def concentration(self, value):
        """Sets the concentration at the Node"""
        _states[self._index] = value



class Node3D(Node):
    def __init__(self, index, i, j, k, r, seg):
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
        self._r = r
        self._seg = seg
    
    @property
    def _ref_concentration(self):
        raise Exception('need to reimplement _ref_concentration for 3D')
    
    @property
    def concentration(self):
        return _states[self._index]
    
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
        raise Exception('need to reimplement x for 3d nodes')
    
    @property
    def seg(self):
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
        return r._dx ** 3

    @concentration.setter
    def concentration(self, value):
        _states[self._index] = value
        # TODO: transfer this value to the corresponding NEURON nodes, if any

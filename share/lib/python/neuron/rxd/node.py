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
_states = h.Vector()

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
    _states.resize(total)
    return start_index

class Node(object):
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

        i -- the offset into the RxDSection 's data

        location -- the location of the compartment within the section. For @aSection1D objects, this is the normalized position 0 <= x <= 1
        """
        self._sec = sec
        self._location = location
        self._index = i + sec._offset
    
    @property
    def _ref_concentration(self):
        """Returns a HOC reference to the Node's state"""
        #void_p = ctypes.cast(_states.ctypes.data_as(ctypes.POINTER(ctypes.c_double)), ctypes.c_voidp).value + self._index * ctypes.sizeof(ctypes.c_double)
        #return _nrndll.nrn_hocobj_ptr(ctypes.cast(void_p, ctypes.POINTER(ctypes.c_double)))
        if self._sec.nrn_region is not None and self._sec.species.name is not None:
            # return pointer to legacy node, if one exists
            return self._sec._sec(self.x).__getattribute__('_ref_%s%s' % (self._sec.species.name, self._sec.nrn_region))
        else:
            # no legacy node analog, return pointer to rxd array
            return _states._ref_x[self._index]

    def satisfies(self, condition):
        """Tests if a Node satisfies a given condition.

        If a nrn.Section object or RxDSection is provided, returns True if the Node lies in the section; else False.
        If a Region object is provided, returns True if the Node lies in the Region; else False.
        If a number between 0 and 1 is provided, returns True if the normalized position lies within the Node; else False.
        """
        if isinstance(condition, nrn.Section) or isinstance(condition, rxdsection.RxDSection):
            return self._sec.name() == condition.name()
        elif isinstance(condition, region.Region):
            return self.region == condition
        try:
            if 0 < condition <= 1:
                dx = self._sec._dx / 2
                return -dx < self._location - condition <= dx
            elif condition == 0:
                return self._location < self._sec._dx
        except:
            raise Exception('unrecognized node condition: %r' % condition)

    @property
    def volume(self):
        """The volume of the compartment in cubic microns.
        
        Read only."""
        rxd._update_node_data()
        return _volumes[self._index]
    
    @property
    def surface_area(self):
        """The surface area of the compartment in square microns.
        
        This is the area (if any) of the compartment that lies on the plasma membrane
        and therefore is the area used to determine the contribution of currents (e.g. ina) from
        mod files or kschan to the compartment's concentration.
        
        Read only.
        """
        
        rxd._update_node_data()
        return _volumes[self._index]
    
        
    @property
    def x(self):
        """The normalized position of the center of the compartment.
        
        Read only."""
        # TODO: will probably want to change this to be more generic for higher dimensions
        return self._location
    
    @property
    def diff(self):
        """Gets the diffusion rate within the compartment."""
        return _diffs[self._index]
    @diff.setter
    def diff(self, value):
        """Sets the diffusion rate within the compartment."""
        rxd._invalidate_matrices()
        _diffs[self._index] = value
    
    @property
    def region(self):
        """The region containing the compartment."""
        return self._sec.region()
    
    @property
    def sec(self):
        """The RxDSection containing the compartment."""
        return self._sec
    
    @property
    def species(self):
        """The Species whose concentration is recorded at this Node."""
        return self._sec._species()
    
    @property
    def concentration(self):
        """Gets the concentration at the Node."""
        if self._sec.nrn_region is not None and self._sec.species.name is not None:
            # legacy grid value exists, return that one
            return self._sec._sec(self.x).__getattribute__('%s%s' % (self._sec.species.name, self._sec.nrn_region))
        else:
            return _states.x[self._index]
        
    @concentration.setter
    def concentration(self, value):
        """Sets the concentration at the Node."""
        _states.x[self._index] = value
        
        if self._sec.nrn_region is not None and self._sec.species.name is not None:
            # TODO: this needs modified for 3d to only change part of the legacy concentration
            self._sec._sec(self.x).__setattr__(self._sec.species.name + self._sec.nrn_region, value)

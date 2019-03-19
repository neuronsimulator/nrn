from .rxdmath import _Arithmeticed
import weakref
from .section1d import Section1D
from neuron import h, nrn, nrn_dll_sym
from . import node, nodelist, rxdmath, region
import numpy
import warnings
import itertools
from .rxdException import RxDException
from . import initializer
import collections
import ctypes

#Now set in rxd.py
#set_nonvint_block = neuron.nrn_dll_sym('set_nonvint_block')

fptr_prototype = ctypes.CFUNCTYPE(None)

set_setup = nrn_dll_sym('set_setup')
set_setup.argtypes = [fptr_prototype]
set_initialize = nrn_dll_sym('set_initialize')
set_initialize.argtypes = [fptr_prototype]

#setup_solver = nrn.setup_solver
#setup_solver.argtypes = [ctypes.py_object, ctypes.py_object, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double]

insert = nrn_dll_sym('insert')
insert.argtypes = [ctypes.c_int, ctypes.py_object, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.py_object, ctypes.py_object, ctypes.c_int, ctypes.c_double, ctypes.c_double]

insert.restype = ctypes.c_int

species_atolscale = nrn_dll_sym('species_atolscale')
species_atolscale.argtypes = [ctypes.c_int, ctypes.c_double, ctypes.c_int, ctypes.POINTER(ctypes.c_int)]

_set_grid_concentrations = nrn_dll_sym('set_grid_concentrations')
_set_grid_concentrations.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.py_object, ctypes.py_object]

_set_grid_currents = nrn_dll_sym('set_grid_currents')
_set_grid_currents.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.py_object, ctypes.py_object, ctypes.py_object]

_delete_by_id = nrn_dll_sym('delete_by_id')
_delete_by_id.argtypes = [ctypes.c_int]


# The difference here is that defined species only exists after rxd initialization
_all_species = []
_defined_species = {}
def _get_all_species():
    return _defined_species

_extracellular_diffusion_objects = weakref.WeakKeyDictionary()

_species_count = 0

_species_on_regions = []

_has_1d = False
_has_3d = False

def _1d_submatrix_n():
    if not _has_1d:
        return 0
    elif not _has_3d:
        return len(node._states)
    else:
        return numpy.min([sp()._indices3d() for sp in list(_get_all_species().values()) if sp() is not None])
            
_extracellular_has_setup = False
_extracellular_exists = False

def _extracellular_do_setup():
    global _extracellular_has_setup
    _extracellular_has_setup = True


def _extracellular_do_initialize():
    if _extracellular_has_setup:
        """handle initialization at finitialize time"""
        for obj in _extracellular_diffusion_objects:
            obj._finitialize()
        # TODO: allow


_extracellular_set_setup = nrn_dll_sym('set_setup')
_extracellular_set_setup.argtypes = [fptr_prototype]
_extracellular_set_initialize = nrn_dll_sym('set_initialize')
_extracellular_set_initialize.argtypes = [fptr_prototype]

def _ensure_extracellular():
    global _extracellular_exists, do_setup_fptr, do_initialize_fptr
    if not _extracellular_exists:
        do_setup_fptr = fptr_prototype(_extracellular_do_setup)
        do_initialize_fptr = fptr_prototype(_extracellular_do_initialize)
        _extracellular_set_setup(do_setup_fptr)
        _extracellular_set_initialize(do_initialize_fptr)
    _extracellular_exists = True



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
    def __radd__(self, other):
        return _Arithmeticed(other) + self
    def __rmul__(self, other):
        return _Arithmeticed(self) * other
    def __rdiv__(self, other):
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
        return rxdmath._Reaction(self2, other, '<>')
    
    def __gt__(self, other):
        other = rxdmath._ensure_arithmeticed(other)
        self2 = rxdmath._ensure_arithmeticed(self)
        rxdmath._validate_reaction_terms(self2, other)
        return rxdmath._Reaction(self2, other, '>')

    def __lt__(self, other):
        other = rxdmath._ensure_arithmeticed(other)
        self2 = rxdmath._ensure_arithmeticed(self)
        rxdmath._validate_reaction_terms(self2, other)
        return rxdmath._Reaction(self2, other, '<')

    @property
    def _semi_compile(self):
        return 'species[%d][]' % (self._id)

    def _involved_species(self, the_dict):
        the_dict[self._semi_compile] = weakref.ref(self)

    @property
    def d(self):
        """diffusion constant. write-only"""
        raise RxDException('diffusion constant is write-only')
    
    @d.setter
    def d(self, value):
        from . import rxd
        if hasattr(self, '_allow_setting'):
            self._d = value
        else:
            _volumes, _surface_area, _diffs = node._get_data()
            _diffs[self.indices()] = value
            rxd._setup_matrices()

class SpeciesOnExtracellular(_SpeciesMathable):
    def __init__(self, species, extracellular):
        """The restriction of a Species to a Region."""
        self._species = weakref.ref(species)
        self._extracellular = weakref.ref(extracellular)
        self._id = _species_count
    

    @property
    def states3d(self):
        return self._extracellular().states

    def extent(self, axes):
        """valid options for axes: xyz, xy, xz, yz, x, y, z"""
        e = self._extracellular()
        if axes == 'xyz':
            return [e._xlo, e._xhi, e._ylo, e._yhi, e._zlo, e._zhi]
        elif axes == 'xy':
            return [e._xlo, e._xhi, e._ylo, e._yhi]
        elif axes == 'xz':
            return [e._xlo, e._xhi, e._zlo, e._zhi]
        elif axes == 'yz':
            return [e._ylo, e._yhi, e._zlo, e._zhi]
        elif axes == 'x':
            return [e._xlo, e._xhi]
        elif axes == 'y':
            return [e._ylo, e._yhi]
        elif axes == 'z':
            return [e._zlo, e._zhi]
        else:
            raise RxDException('unknown axes argument; valid options: xyz, xy, xz, yz, x, y and z')

    def node_by_location(self,x,y,z):
        e = self._extracellular()._region
        i = int((x - e._xlo) / e._dx[0])
        j = int((y - e._ylo) / e._dx[1])
        k = int((z - e._zlo) / e._dx[2])
        return self.node_by_ijk(i,j,k) 

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
            alphas.append(e.alpha[i,j,k])
        return numpy.array(alphas)
    
    def node_by_ijk(self,i,j,k):
        index = 0
        s = self._extracellular()
        for ecs in self._species()._extracellular_instances:
            if ecs == s:
                e = s._region
                index += (i * e._ny + j) * e._nz + k
            else:
                e = ecs._region
                index += e._nz * e._ny * e._nz
        return self._species()._extracellular_nodes[index]
 

                
    @property
    def _semi_compile(self):
        return 'species_ecs[%d]' % (self._extracellular()._grid_id)

class SpeciesOnRegion(_SpeciesMathable):
    def __init__(self, species, region):
        """The restriction of a Species to a Region."""
        global _species_on_regions
        self._species = weakref.ref(species)
        self._region = weakref.ref(region)
        if hasattr(species,'_id'):  
            self._id = species._id
        _species_on_regions.append(weakref.ref(self))
        
    def __eq__(self, other):
        """test for equality.
        
        Two SpeciesOnRegion objects are equal if they refer to the same species
        on the same region and both the species and the region still exist.
        """
        return (self._species() == other._species()) and (self._region() == other._region()) and (self._species() is not None) and (self._region() is not None)
    
    def __hash__(self):
        # TODO: replace this to reduce collision risk; how much of a problme is that?
        return 1000 * (hash(self._species()) % 1000) + (hash(self._region()) % 1000)
    
    
    def __repr__(self):
        return '%r[%r]' % (self._species(), self._region())
        
    def _short_repr(self):
        return '%s[%s]' % (self._species()._short_repr(), self._region()._short_repr())
    def __str__(self):
        return '%s[%s]' % (self._species()._short_repr(), self._region()._short_repr())

    def indices(self, r=None, secs=None):
        """If no Region is specified or if r is the Region specified in the constructor,
        returns a list of the indices of state variables corresponding
        to the Species when restricted to the Region defined in the constructor.

        If r is a different Region, then returns the empty list.
        If secs is a set return all indices on the regions for those sections.
        """    
        #if r is not None and r != self._region():
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
        else:
            return SpeciesOnRegion(self._species, None)
    
    @property
    def states(self):
        """A vector of all the states corresponding to this species"""
        all_states = node._get_states()
        return [all_states[i] for i in numpy.sort(self.indices())]
    
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
        initializer._do_init()
        return nodelist.NodeList(itertools.chain.from_iterable([s.nodes for s in self._species()._secs if s._region == self._region()]))
    
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
    def _semi_compile(self):
        return 'species[%d][%d]' % (self._id, self._region()._id)

# 3d matrix stuff
def _setup_matrices_process_neighbors(pt1, pt2, indices, euler_matrix, index, diffs, vol, areal, arear, dx):
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
    n3d = int(h.n3d(sec=sec))
    x3d = [h.x3d(i, sec=sec) for i in range(n3d)]
    y3d = [h.y3d(i, sec=sec) for i in range(n3d)]
    z3d = [h.z3d(i, sec=sec) for i in range(n3d)]
    arc3d = [h.arc3d(i, sec=sec) for i in range(n3d)]
    return numpy.interp([seg.x * sec.L], arc3d, x3d)[0], numpy.interp([seg.x * sec.L], arc3d, y3d)[0], numpy.interp([seg.x * sec.L], arc3d, z3d)[0]




class _ExtracellularSpecies(_SpeciesMathable):
    def __init__(self, region, d=0, name=None, charge=0, initial=0, atolscale=1.0, boundary_conditions=None):
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

        if(numpy.isscalar(region.alpha)):
            self.alpha = self._alpha = region.alpha
        else:
            self.alpha = region.alpha
            self._alpha = region._alpha._ref_x[0]
        
        if(numpy.isscalar(region.tortuosity)):
            self.tortuosity = self._tortuosity = region.tortuosity
        else:
            self.tortuosity = region.tortuosity
            self._tortuosity = region._tortuosity._ref_x[0]

        if boundary_conditions is None:
            bc_type = 0
            bc_value = 0.0
        else:
            bc_type = 1
            bc_value = boundary_conditions
        
        # TODO: if allowing different diffusion rates in different directions, verify that they go to the right ones
        if not hasattr(self._d,'__len__'):
            self._grid_id = insert(0, self._states._ref_x[0], self._nx, self._ny, self._nz, self._d, self._d, self._d, self._dx[0], self._dx[1], self._dx[2], self._alpha, self._tortuosity, bc_type, bc_value, atolscale)
        elif len(self._d) == 3:
             self._grid_id = insert(0, self._states._ref_x[0], self._nx, self._ny, self._nz, self._d[0], self._d[1], self._d[2], self._dx[0], self._dx[1], self._dx[2], self._alpha, self._tortuosity, bc_type, bc_value, atolscale)
        else:
            raise RxDException("Diffusion coefficient %s for %s is invalid. A single value D or a tuple of 3 values (Dx,Dy,Dz) is required for the diffusion coefficient." % (repr(d), name))  

        self._name = name


        # set up the ion mechanism and enable active Nernst potential calculations
        self._ion_register()

        self._update_pointers()

    def __del__(self):
        # TODO: remove this object from the list of grids, possibly by reinserting all the others
        # NOTE: be careful about doing the right thing at program's end; some globals may no longer exist
        try:
            if self in _extracellular_diffusion_objects: del _extracellular_diffusion_objects[self]
            # remove the grid id
            if _extracellular_diffusion_objects:
                for sp in _extracellular_diffusion_objects:
                    if hasattr(sp,'_grid_id') and sp._grid_id > self._grid_id:
                        sp._grid_id -= 1
            if hasattr(self,'_grid_id'): _delete_by_id(self._grid_id)
            
            nrn_dll_sym('structure_change_cnt', ctypes.c_int).value += 1
        except:
            return
            
    def _finitialize(self):
        # Updated - now it will initialize using NodeExtracellular
        if self._initial is None:
            if hasattr(h,'%so0_%s_ion' % (self._species, self._species)):
                self.states[:] = getattr(h,'%so0_%s_ion' % (self._species, self._species))
            else:
                self.states[:] = 0

    def _ion_register(self):
        """modified from neuron.rxd.species.Species._ion_register"""
        ion_type = h.ion_register(self._species, self._charge)
        if ion_type == -1:
            raise RxDException('Unable to register species: %s' % self._species)
        # insert the species if not already present
        ion_forms = [self._species + 'i', self._species + 'o', 'i' + self._species, 'e' + self._species]
        for s in h.allsec():
            try:
                for i in ion_forms:
                    # this throws an exception if one of the ion forms is missing
                    temp = s.__getattribute__(i)
            except:
                s.insert(self._species + '_ion')
            # set to recalculate reversal potential automatically
            # the last 1 says to set based on global initial concentrations
            # e.g. nao0_na_ion, etc...
            # TODO: this is happening GLOBALLY even to sections that aren't inside the extracellular domain (FIX THIS)
            h.ion_style(self._species + '_ion', 3, 2, 1, 1, 1, sec=s)

    def _nodes_by_location(self, i, j, k):
        return (i * self._ny + j) * self._nz + k

    def index_from_xyz(self, x, y, z):
        """Given an (x, y, z) point, return the index into the _species vector containing it or None if not in the domain"""
        if x < self._xlo or x > self._xhi or y < self._ylo or y > self._yhi or z < self._zlo or z > self._zhi:
            return None
        # if we make it here, then we are inside the domain
        i = int((x - self._xlo) / self._dx[0])
        j = int((y - self._ylo) / self._dx[1])
        k = int((z - self._zlo) / self._dx[2])
        return self._nodes_by_location(i, j, k)

    def ijk_from_index(self, index):
        nynz = self._ny * self._nz
        i = int(index / nynz)
        jk = index - nynz * i
        j = int(jk / self._nz)
        k = jk % self._nz
        # sanity check
        assert(index == self._nodes_by_location(i, j, k))
        return i, j, k

    def xyz_from_index(self, index):
        i, j, k = ijk_from_index(self, index)
        return self._xlo + i * self._dx[0], self._ylo + j * self._dx[1], self._zlo + k * self._dx[2]

    def _locate_segments(self):
        """Note: there may be Nones in the result if a section extends outside the extracellular domain

        Note: the current version keeps all the sections alive (i.e. they will never be garbage collected)
        TODO: fix this
        """
        result = {}
        for sec in h.allsec():
            result[sec] = [self.index_from_xyz(*_xyz(seg)) for seg in sec]
        return result

    def _update_pointers(self):
        # TODO: call this anytime the _grid_id changes and anytime the structure_change_count changes
        self._seg_indices = self._locate_segments()
        from .geometry import _surface_areas1d

        grid_list = 0
        grid_indices = []
        neuron_pointers = []
        stateo = '_ref_' + self._species + 'o'
        for sec, indices in self._seg_indices.items():
            for seg, i in zip(sec, indices):
                if i is not None:
                    grid_indices.append(i)
                    neuron_pointers.append(seg.__getattribute__(stateo))
        _set_grid_concentrations(grid_list, self._grid_id, grid_indices, neuron_pointers)

        tenthousand_over_charge_faraday = 10000. / (self._charge * h.FARADAY)
        scale_factor = tenthousand_over_charge_faraday / (numpy.prod(self._dx))
        ispecies = '_ref_i' + self._species
        neuron_pointers = []
        scale_factors = []
        for sec, indices in self._seg_indices.items():
            for seg, surface_area, i in zip(sec, _surface_areas1d(sec), indices):
                if i is not None:
                    neuron_pointers.append(seg.__getattribute__(ispecies))
                    scale_factors.append(float(scale_factor * surface_area))
        #TODO: MultiCompartment reactions ?
        _set_grid_currents(grid_list, self._grid_id, grid_indices, neuron_pointers, scale_factors)
    @property
    def _semi_compile(self):
        return 'species_ecs[%d]' % (self._grid_id)




# TODO: make sure that we can make this work where things diffuse across the
#       boundary between two regions... for the tree solver, this is complicated
#       because need the sections in a sorted order
#       ... but this is also weird syntactically, because how should it know
#       that two regions (e.g. apical and other_dendrites) are connected?

class Species(_SpeciesMathable):
    def __init__(self, regions=None, d=0, name=None, charge=0, initial=None, atolscale=1, ecs_boundary_conditions=None):
        """s = rxd.Species(regions, d = 0, name = None, charge = 0, initial = None, atolscale=1)
    
        Declare a species.

        Parameters:

        regions -- a Region or list of Region objects containing the species

        d -- the diffusion constant of the species (optional; default is 0, i.e. non-diffusing)

        name -- the name of the Species; used for syncing with HOC (optional; default is none)

        charge -- the charge of the Species (optional; default is 0)

        initial -- the initial concentration or None (if None, then imports from HOC if the species is defined at finitialize, else 0)

        atolscale -- scale factor for absolute tolerance in variable step integrations

        ecs_boundary_conditions  -- if Extracellular rxd is used ecs_boundary_conditions=None for zero flux boundaries or if ecs_boundary_conditions=the concentration at the boundary.
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
        self.d = d
        self.name = name
        self.charge = charge
        self.initial = initial
        self._atolscale = atolscale
        self._ecs_boundary_conditions = ecs_boundary_conditions
        _all_species.append(weakref.ref(self))
        # declare an update to the structure of the model (the number of differential equations has changed)
        nrn_dll_sym('structure_change_cnt', ctypes.c_int).value += 1

        # initialize self if the rest of rxd is already initialized
        if initializer.is_initialized():
            if _has_3d:
                if isinstance(regions, region.Region) and not regions._secs1d:
                    pass
                elif hasattr(regions, '__len__') and all(not r._secs1d for r in regions):
                    pass
                else:
                    # TODO: remove this limitation
                    #       one strategy would be to just redo the whole thing; what are the implications of that?
                    #       (pointers would be invalid; anything else?)
                    raise RxDException('Currently cannot add species containing 1D after 3D species defined and initialized. To work-around: reorder species definition.')
            self._do_init()
    
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
                raise RxDException('Species name must be a string')
            if name in _defined_species and _defined_species[name]() is not None:
                raise RxDException('Species "%s" previously defined: %r' % (name, _defined_species[name]()))
        else:
            name = _species_count
        self._id = _species_count
        _species_count += 1
        _defined_species[name] = weakref.ref(self)
        if regions is None:
            raise RxDException('Must specify region where species is present')
        if hasattr(regions, '__len__'):
            regions = list(regions)
        else:
            regions = list([regions])
        # TODO: unite handling of _regions and _extracellular_regions
        self._regions = [r for r in regions if not isinstance(r, region.Extracellular)]
        self._extracellular_regions = [r for r in regions if isinstance(r, region.Extracellular)]
        if not all(isinstance(r, region.Region) for r in self._regions):
            raise RxDException('regions list must consist of Region and Extracellular objects only')
        if self._extracellular_regions:
            # make sure that the extracellular callbacks are configured, if necessary
            _ensure_extracellular()
        self._species = weakref.ref(self)        
        # at this point self._name is None if unnamed or a string == name if
        # named
        self._ion_register()                     
        
        # TODO: remove this line when certain no longer need it (commented out 2013-04-17)
        # self._real_secs = region._sort_secs(sum([r.secs for r in regions], []))

        #Set the SpeciesOnRegion id
        for sp in _species_on_regions:
            s = sp()
            if s and s._species() == self:
                s._id = self._id
    
    def _do_init2(self):
        # 1D section objects; NOT all sections this species lives on
        # TODO: this may be a problem... debug thoroughly
        global _has_1d
        d = self._d

        self._secs = [Section1D(self, sec, d, r) for r in self._regions for sec in r._secs1d]
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
        self._nodes = []
        selfref = weakref.ref(self)
        self_has_3d = False
        if self._regions:
            for r in self._regions:
                if r._secs3d:
                    xs, ys, zs, segs = r._xs, r._ys, r._zs, r._segs
                    if not self_has_3d:
                        self._3doffset = node._allocate(len(xs))
                        _3doffset = self._3doffset
                    else:
                        _3doffset = node._allocate(len(xs))
                    self._3doffset_by_region[r] = _3doffset
                    
                    for i, x, y, z, seg in zip(list(range(len(xs))), xs, ys, zs, segs):
                        self._nodes.append(node.Node3D(i + _3doffset, x, y, z, r, seg, selfref))
                    # the region is now responsible for computing the correct volumes and surface areas
                        # this is done so that multiple species can use the same region without recomputing it
                    node._volumes[_3doffset : _3doffset + len(xs)] = r._vol
                    node._surface_area[_3doffset : _3doffset + len(xs)] = r._sa
                    node._diffs[_3doffset : _3doffset + len(xs)] = self._d
                    self_has_3d = True
                    _has_3d = True

    def _do_init4(self):
        self._extracellular_nodes = []
        self._extracellular_instances = [_ExtracellularSpecies(r, d=self._d, name=self.name, charge=self.charge, initial=self.initial, atolscale=self._atolscale, boundary_conditions=self._ecs_boundary_conditions) for r in self._extracellular_regions]
        sp_ref = weakref.ref(self)
        for r in self._extracellular_regions:
            for i in range(r._nx):
                for j in range(r._ny):
                    for k in range(r._nz):
                        self._extracellular_nodes.append(node.NodeExtracellular((i * r._ny + j) * r._nz + k, i, j, k, r, sp_ref, weakref.ref(r)))
           
    
    def _do_init5(self):
        # final initialization
        for sec in self._secs:
            # NOTE: can only init_diffusion_rates after the roots (parents)
            #       have been assigned
            sec._init_diffusion_rates()
        self._update_node_data()    # is this really nessesary
        self._update_region_indices()

    def _do_init6(self):
        self._register_cptrs()

    def __del__(self):
        if not weakref or not weakref.ref:
            # probably at exit -- not worth tidying up
            return
        
        global _all_species, _defined_species
        try:
            from . import section1d, node
        except:
            # may not be able to import on exit
            return 
        
        if self.name in _defined_species: del _defined_species[self.name]
        _all_species = list(filter(lambda x: x() is not None or x() == self, _all_species))
        # delete the secs
        if hasattr(self,'_secs') and self._secs:
            # remove the species root
            # TODO: will this work with post initialization morphology changes
            if self._has_adjusted_offsets:
                self._secs[0]._nseg += self._num_roots
                self._secs[0]._offset -= self._num_roots
            self._secs.sort(key=lambda s: s._offset, reverse=True)
            for sec in self._secs:
                # node data is removed here in case references to sec remains
                node._remove(sec._offset, sec._offset + sec._nseg + 1)
                # shift offset to account for deleted sec
                for secs in section1d._rxd_sec_lookup.values():
                    for s in secs:
                        if s() and s()._offset > sec._offset:
                            s()._offset -= sec._nseg + 1
                del sec
        # set the remaining species offsets
        for sr in _all_species:
            s = sr()
            if s is not None:
                s._update_region_indices(True)
        
    def _ion_register(self):
        charge = self._charge
        if self._name is not None:
            ion_type = h.ion_register(self._name, charge)
            if ion_type == -1:
                raise RxDException('Unable to register species: %s' % self._name)
            # insert the species if not already present
            regions = self._regions if hasattr(self._regions, '__len__') else [self._regions]
            for r in regions:
                if r.nrn_region in ('i', 'o'):
                    for s in r.secs:
                        try:
                            ion_forms = [self._name + 'i', self._name + 'o', 'i' + self._name, 'e' + self._name]
                            for i in ion_forms:
                                # this throws an exception if one of the ion forms is missing
                                temp = s.__getattribute__(self._name + 'i')
                        except:
                            s.insert(self._name + '_ion')
                        # set to recalculate reversal potential automatically
                        # the last 1 says to set based on global initial concentrations
                        # e.g. nai0_na_ion, etc...
                        h.ion_style(self._name + '_ion', 3, 2, 1, 1, 1, sec=s)

    @property
    def states(self):
        """A vector of all the states corresponding to this species"""
        all_states = node._get_states()
        return [all_states[i] for i in numpy.sort(self.indices())]


    def _setup_matrices3d(self, euler_matrix):
        for r in self._regions:
            region_mesh = r._mesh.values
            indices = {}
            xs, ys, zs = region_mesh.nonzero()
            diffs = node._diffs
            offset = self._3doffset_by_region[r]
            for i in range(len(xs)):
                indices[(xs[i], ys[i], zs[i])] = i + offset
            dx = self._regions[0]._dx
            naf = self._regions[0]._geometry.neighbor_area_fraction
            if not isinstance(naf, collections.Callable):
                areazl = areazr = areayl = areayr = areaxl = areaxr = dx * dx * naf 
                for nodeobj in self._nodes:
                    i, j, k, index, vol = nodeobj._i, nodeobj._j, nodeobj._k, nodeobj._index, nodeobj.volume
                    _setup_matrices_process_neighbors((i, j, k - 1), (i, j, k + 1), indices, euler_matrix, index, diffs, vol, areazl, areazr, dx)
                    _setup_matrices_process_neighbors((i, j - 1, k), (i, j + 1, k), indices, euler_matrix, index, diffs, vol, areayl, areayr, dx)
                    _setup_matrices_process_neighbors((i - 1, j, k), (i + 1, j, k), indices, euler_matrix, index, diffs, vol, areaxl, areaxr, dx)
            else:
                for nodeobj in self._nodes:
                    i, j, k, index, vol = nodeobj._i, nodeobj._j, nodeobj._k, nodeobj._index, nodeobj.volume
                    areaxl, areaxr, areayl, areayr, areazl, areazr = naf(i, j, k)
                    _setup_matrices_process_neighbors((i, j, k - 1), (i, j, k + 1), indices, euler_matrix, index, diffs, vol, areazl, areazr, dx)
                    _setup_matrices_process_neighbors((i, j - 1, k), (i, j + 1, k), indices, euler_matrix, index, diffs, vol, areayl, areayr, dx)
                    _setup_matrices_process_neighbors((i - 1, j, k), (i + 1, j, k), indices, euler_matrix, index, diffs, vol, areaxl, areaxr, dx)


    def re_init(self):
        """Reinitialize the rxd concentration of this species to match the NEURON grid"""
        self._import_concentration(init=False)

    def __getitem__(self, r):
        """Return a reference to those members of this species lying on the specific region @varregion.
        The resulting object is a SpeciesOnRegion.
        This is useful for defining reaction schemes for MultiCompartmentReaction."""
        if isinstance(r, region.Region):
            return SpeciesOnRegion(self, r)
        elif isinstance(r, region.Extracellular):
            if not hasattr(self,'_extracellular_instances'):
                initializer._do_init()
            for e in self._extracellular_instances:
                if e._region == r:
                    return SpeciesOnExtracellular(self, e)
        raise RxDException('no such region')

    def _update_node_data(self):
        nsegs_changed = 0
        for sec in self._secs:
            nsegs_changed += sec._update_node_data()
        return nsegs_changed

    def concentrations(self):
        if self._secs:
            raise RxDException('concentrations only supports 3d and that is deprecated')
        warnings.warn('concentrations is deprecated; do not use')
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
        species_atolscale(self._id, self._atolscale, len(idx), (ctypes.c_int * len(idx))(*idx))
        # 3D stuff
        self._concentration_ptrs = []
        self._seg_order = []
        if self._nodes and self.name is not None:
            for r in self._regions:
                nrn_region = r._nrn_region
                if nrn_region is not None:
                    ion = '_ref_' + self.name + nrn_region
                    current_region_segs = list(r._nodes_by_seg.keys())
                    self._seg_order += current_region_segs
                    for seg in current_region_segs:
                        self._concentration_ptrs.append(seg.__getattribute__(ion))    

    @property
    def charge(self):
        """Get or set the charge of the Species.
        
        .. note:: Setting is new in NEURON 7.4+ and is allowed only before the reaction-diffusion model is instantiated.
        """
        return self._charge

    @charge.setter
    def charge(self, value):
        if hasattr(self, '_allow_setting'):
            self._charge = value
        else:
            raise RxDException('Cannot set charge now; model already instantiated')

    @property
    def regions(self):
        """Get or set the regions where the Species is present
        
        .. note:: New in NEURON 7.4+. Setting is allowed only before the reaction-diffusion model is instantiated.

        .. note:: support for getting the regions is new in NEURON 7.5.
        """
        return list(self._regions) + list(self._extracellular_regions)

    @regions.setter
    def regions(self, regions):
        if hasattr(self, '_allow_setting'):
            if hasattr(regions, '__len__'):
                regions = list(regions)
            else:
                regions = list([regions])
            self._regions = [r for r in regions if not isinstance(r, region.Extracellular)]
            self._extracellular_regions = [r for r in regions if isinstance(r, region.Extracellular)]
        else:
            raise RxDException('Cannot set regions now; model already instantiated')

        
    def _update_region_indices(self, update_offsets=False):
        # TODO: should this include 3D nodes? currently 1D only. (What faces the user?)
        # update offset in case nseg has changed
        if hasattr(self,'_secs') and self._secs and update_offsets:
            self._offset = self._secs[0]._offset - self._num_roots
        self._region_indices = {}
        for s in self._secs:
            if s._region not in self._region_indices:
                self._region_indices[s._region] = []
            self._region_indices[s._region] += s.indices
        # a list of all indices
        self._region_indices[None] = list(itertools.chain.from_iterable(list(self._region_indices.values())))
    
    def _indices3d(self, r=None):
        """return the indices of just the 3D nodes corresponding to this species in the given region"""
        # TODO: this will need changed if 3D is to support more than one region
        if r is None or r == self._regions[0]:
            return list(range(self._3doffset, self._3doffset + len(self._nodes)))
        else:
            return []

    def _indices1d(self, r=None):
        """return the indices of just the 1D nodes corresponding to this species in the given region"""
        return self._region_indices.get(r, [])
    
    def indices(self, r=None, secs=None):
        """return the indices corresponding to this species in the given region
        
        if r is None, then returns all species indices
        If r is a list of regions return indices for only those sections that are on all the regions.
        If secs is a set return all indices on the regions for those sections. """
        # TODO: beware, may really want self._indices3d or self._indices1d
        initializer._do_init()
        if secs is not None:
            if type(secs) != set:
                secs={secs}
            if r is None:
                regions = self._regions
            elif not hasattr(r,'__len__'):
                regions = [r]
            else:
                regions = r
            return list(itertools.chain.from_iterable([s.indices for s in self._secs if s._sec in secs and s.region in regions]))
                
            """for reg in regions:
                ind = self.indices(reg)
                offset = 0
                for sec in r.secs:
                    if sec not in secs:
                        del ind[offset:(offset+sec.nseg)]
                    offset += sec.nseg
                return ind"""
        else:
            if not hasattr(r,'__len__'):
                return self._indices1d(r) + self._indices3d(r)
            elif len(r) == 1:
                return self._indices1d(r[0]) + self._indices3d(r[0])
            else:   #Find the intersection
                interseg = set.intersection(*[set(reg.secs) for reg in r if reg is not None])
                return self.indices(r,interseg)
    
    def _setup_diffusion_matrix(self, g):
        for s in self._secs:
            s._setup_diffusion_matrix(g)
    
    def _setup_c_matrix(self, c):
        # TODO: this will need to be changed for three dimensions, or stochastic
        for s in self._secs:
            c[s._offset:s._offset + s.nseg] = 1.0
    
    def _setup_currents(self, indices, scales, ptrs, cur_map):
        from . import rxd
        if self.name:
            cur_map[self.name + 'i'] = {}
            cur_map[self.name + 'o'] = {}
        # 1D part
        for s in self._secs:
            s._setup_currents(indices, scales, ptrs, cur_map)
        # 3D part
        if self._nodes:
            # TODO: this is very similar to the 1d code; merge
            if self.name is not None and self.charge != 0:
                ion_curr = '_ref_i%s' % self.name
                volumes, surface_area, diffs = node._get_data()
                # NOTE: this implicitly assumes that o and i border the membrane
                local_indices = self._indices3d()
                offset = self._offset
                charge = self.charge
                namei = self._name + 'i'
                nameo = self._name + 'o'
                tenthousand_over_charge_faraday = 10000. / (charge * rxd.FARADAY)
                for i, nodeobj in enumerate(self._nodes):
                    if surface_area[i]:
                        r = nodeobj.region
                        nrn_region = r.nrn_region
                        if nrn_region == 'i':
                            sign = -1
                            seg = nodeobj.segment
                            cur_map[namei][seg] = len(indices)
                        elif nrn_region == 'o':
                            sign = 1
                            seg = nodeobj.segment
                            cur_map[nameo][seg] = len(indices)
                        else:
                            continue
                        indices.append(local_indices[i])
                        if volumes[i + offset] == 0:
                            print(('0 volume at position %d; surface area there: %g' % (i + offset, surface_area[i + offset])))
                        scales.append(sign * tenthousand_over_charge_faraday * surface_area[i + offset] / volumes[i + offset])
                        ptrs.append(seg.__getattribute__(ion_curr))

    
    def _has_region_section(self, region, sec):
        return any((s._region == region and s._sec == sec) for s in self._secs)

    def _region_section(self, region, sec):
        """return the Section1D (or whatever) associated with the region and section"""
        for s in self._secs:
            if s._region == region and s._sec == sec:
                return s
        else:
            raise RxDException('requested section not in species')

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
            self._has_adjusted_offsets = True
    
    
    def _finitialize(self, skip_transfer=False):
        if hasattr(self,'_extracellular_instances'):
            for r in self._extracellular_instances:
                r._finitialize()
        if self.initial is not None:
            if isinstance(self.initial, collections.Callable):
                for node in self.nodes:
                    node.concentration = self.initial(node)
            else:
                for node in self.nodes:
                    node.concentration = self.initial
        elif self.name is None:
            for node in self.nodes:
                node.concentration = 0
        else:
            self._import_concentration()
            skip_transfer = False
        if not skip_transfer:
            self._transfer_to_legacy() 
    
    def _transfer_to_legacy(self):
        """Transfer concentrations to the standard NEURON grid"""
        if self._name is None: return
        
        # 1D part
        for sec in self._secs: sec._transfer_to_legacy()
        
        # 3D part
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
    
    def _import_concentration(self, init=True):
        """Read concentrations from the standard NEURON grid"""
        if self._name is None: return
        
        # start with the 1D stuff
        for sec in self._secs: sec._import_concentration(init)

        # now the 3D stuff
        nodes = self._nodes
        if nodes:
            # TODO: replace this with a pointer vec for speed
            #       not a huge priority since import happens rarely if at all
            i = 0
            seg_order = self._seg_order
            conc_ptr = self._concentration_ptrs
            for r in self._regions:
                if r._nrn_region is not None:
                    seg, ptr = seg_order[i], conc_ptr[i]
                    i += 1
                    value = ptr[0]
                    for node in r._nodes_by_seg[seg]:
                        nodes[node].concentration = value
    
    @property
    def nodes(self):
        """A NodeList of all the nodes corresponding to the species.
        
        This can then be further restricted using the callable property of NodeList objects."""

        initializer._do_init()
        
        # The first part here is for the 1D -- which doesn't keep live node objects -- the second part is for 3D
        return nodelist.NodeList(list(itertools.chain.from_iterable([s.nodes for s in self._secs])) + self._nodes + self._extracellular_nodes)


    @property
    def name(self):
        """Get or set the name of the Species.

        This is used only for syncing with the non-reaction-diffusion parts of NEURON.
        
        .. note:: Setting only supported in NEURON 7.4+, and then only before the reaction-diffusion model is instantiated.
        """
        return self._name
    
    @name.setter
    def name(self, value):
        if hasattr(self, '_allow_setting'):
            self._name = value
        else:
            raise RxDException('Cannot set name now; model already instantiated')


    def __repr__(self):
        return 'Species(regions=%r, d=%r, name=%r, charge=%r, initial=%r)' % (self._regions, self._d, self._name, self._charge, self.initial)
    
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
    if hasattr(indices,'count'):
        index = indices
    else:
        index = [indices]
    #TODO: make sure to include Node3D
    return [[nd.x3d, nd.y3d, nd.z3d] for sp in _get_all_species().values() for s in sp()._secs for nd in s.nodes + sp()._nodes if sp() if nd._index in index]


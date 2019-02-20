from neuron import h, nrn, nrn_dll_sym 
from . import species, node, section1d, region
from .nodelist import NodeList
import weakref
import numpy
import ctypes
import atexit
from . import options
from .rxdException import RxDException
from . import initializer 
import collections
import os
from distutils import sysconfig
import uuid
import sys
import itertools
from numpy.ctypeslib import ndpointer
import re
import platform
# aliases to avoid repeatedly doing multiple hash-table lookups
_numpy_array = numpy.array
_numpy_zeros = numpy.zeros
_species_get_all_species = species._get_all_species
_node_get_states = node._get_states
_section1d_transfer_to_legacy = section1d._transfer_to_legacy
_ctypes_c_int = ctypes.c_int
_weakref_ref = weakref.ref

_external_solver = None
_external_solver_initialized = False
_windows_dll_files = []
_windows_dll = []



make_time_ptr = nrn_dll_sym('make_time_ptr')
make_time_ptr.argtypes = [ctypes.py_object, ctypes.py_object]
make_time_ptr(h._ref_dt, h._ref_t)

_double_ptr = ctypes.POINTER(ctypes.c_double)
_int_ptr = ctypes.POINTER(_ctypes_c_int)
_long_ptr = ctypes.POINTER(ctypes.c_long)


fptr_prototype = ctypes.CFUNCTYPE(None)
set_nonvint_block = nrn_dll_sym('set_nonvint_block')
set_nonvint_block(nrn_dll_sym('rxd_nonvint_block'))

set_setup = nrn_dll_sym('set_setup')
set_setup.argtypes = [fptr_prototype]
set_initialize = nrn_dll_sym('set_initialize')
set_initialize.argtypes = [fptr_prototype]

scatter_concentrations = nrn_dll_sym('scatter_concentrations')

# Transfer extracellular concentrations to NEURON
_fih_transfer_ecs = h.FInitializeHandler(1, scatter_concentrations)


rxd_set_no_diffusion = nrn_dll_sym('rxd_set_no_diffusion')

setup_solver = nrn_dll_sym('setup_solver')
setup_solver.argtypes = [ndpointer(ctypes.c_double), ctypes.c_int,  numpy.ctypeslib.ndpointer(numpy.int_, flags='contiguous'), ctypes.c_int, ctypes.py_object, ctypes.py_object]

#states = None
_set_num_threads = nrn_dll_sym('set_num_threads')
_set_num_threads.argtypes = [ctypes.c_int]
_get_num_threads = nrn_dll_sym('get_num_threads')
_get_num_threads.restype = ctypes.c_int


clear_rates = nrn_dll_sym('clear_rates')
register_rate = nrn_dll_sym('register_rate')
register_rate.argtypes = [ 
        ctypes.c_int,                                                               #num species
        ctypes.c_int,                                                               #num regions
        ctypes.c_int,                                                               #num seg
        numpy.ctypeslib.ndpointer(ctypes.c_int, flags='contiguous'),                #species ids
        ctypes.c_int, numpy.ctypeslib.ndpointer(ctypes.c_int, flags='contiguous'),  #num ecs species
        numpy.ctypeslib.ndpointer(ctypes.c_int, flags='contiguous'),                #ecs species ids
        ctypes.c_int,                                                               #num multicompartment reactions
        numpy.ctypeslib.ndpointer(ctypes.c_double, flags='contiguous'),             #multicompartment multipliers
        ]                                                                           #Reaction rate function

setup_currents = nrn_dll_sym('setup_currents')
setup_currents.argtypes = [
    ctypes.c_int,   #number of membrane currents
    ctypes.c_int,   #number induced currents
    ctypes.c_int,   #number of nodes with membrane currents
    _int_ptr,       #number of species involved in each membrane current
    _int_ptr,       #charges of the species involved in each membrane current
    _int_ptr,       #node indices
    _int_ptr,       #node indices
    _double_ptr,    #scaling (areas) of the fluxes
    _int_ptr,       #charges for each species in each reation
    ctypes.POINTER(ctypes.py_object),    #hoc pointers
    _int_ptr,       #maps for membrane fluxes
    _int_ptr        #maps for ecs fluxes
]
    

set_reaction_indices = nrn_dll_sym('set_reaction_indices')
set_reaction_indices.argtypes = [ctypes.c_int, _int_ptr, _int_ptr, _int_ptr, 
    _int_ptr,_int_ptr,_double_ptr, ctypes.c_int, _int_ptr, _int_ptr, _int_ptr,
    _int_ptr]

ecs_register_reaction = nrn_dll_sym('ecs_register_reaction')
ecs_register_reaction.argtype = [ctypes.c_int, ctypes.c_int, _int_ptr, fptr_prototype]

set_euler_matrix = nrn_dll_sym('rxd_set_euler_matrix')
set_euler_matrix.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    _long_ptr,
    _long_ptr,
    _double_ptr,
    numpy.ctypeslib.ndpointer(numpy.int_, flags='contiguous'),
    ctypes.c_int,
    numpy.ctypeslib.ndpointer(numpy.double, flags='contiguous'),
]
rxd_setup_curr_ptrs = nrn_dll_sym('rxd_setup_curr_ptrs')
rxd_setup_curr_ptrs.argtypes = [
    ctypes.c_int,
    _int_ptr,
    numpy.ctypeslib.ndpointer(numpy.double, flags='contiguous'),
    ctypes.POINTER(ctypes.py_object),
]

rxd_setup_conc_ptrs = nrn_dll_sym('rxd_setup_conc_ptrs')
rxd_setup_conc_ptrs.argtypes = [
    ctypes.c_int,
    _int_ptr,
    ctypes.POINTER(ctypes.py_object)
]

_c_headers = """#include <math.h>
/*Some functions supported by numpy that aren't included in math.h
 * names and arguments match the wrappers used in rxdmath.py
 */
double factorial(const double);
double degrees(const double);
void radians(const double, double*);
double log1p(const double);
"""

def _list_to_cint_array(data):
    if data is None or len(data) == 0:
        return None
    else:
        return (ctypes.c_int * len(data))(*tuple(data))

def _list_to_cdouble_array(data):
    if data is None or len(data) == 0:
        return None
    else:
        return (ctypes.c_double * len(data))(*tuple(data))

def _list_to_clong_array(data):
    if data is None or len(data) == 0:
        return None
    else:
        return (ctypes.c_long * len(data))(*tuple(data))

def _list_to_pyobject_array(data):
    if data is None or len(data) == 0:
        return None
    else:
        return (ctypes.py_object * len(data))(*tuple(data))

def byeworld():
    # needed to prevent a seg-fault error at shutdown in at least some
    # combinations of NEURON and Python, which I think is due to objects
    # getting deleted out-of-order
    global _react_matrix_solver
    try:
        del _react_matrix_solver
    except NameError:
    #    # if it already didn't exist, that's fine
        pass
    _windows_remove_dlls()
    
atexit.register(byeworld)

# Faraday's constant (store to reduce number of lookups)
FARADAY = h.FARADAY

# converting from mM um^3 to molecules
# = 6.02214129e23 * 1000. / 1.e18 / 1000
# = avogadro * (L / m^3) * (m^3 / um^3) * (mM / M)
# value for avogardro's constant from NIST webpage, accessed 25 April 2012:
# http://physics.nist.gov/cgi-bin/cuu/Value?na
_conversion_factor = 602214.129


_cvode_object = h.CVode()

last_diam_change_cnt = None
last_structure_change_cnt = None

_linmodadd_c = None
_diffusion_matrix = None
_curr_scales = None
_curr_ptrs = None
_curr_indices = None

_all_reactions = []

_zero_volume_indices = numpy.ndarray(0, dtype=numpy.int_)
_nonzero_volume_indices = []

nrn_tree_solve = nrn_dll_sym('nrn_tree_solve')
nrn_tree_solve.restype = None

_dptr = _double_ptr

_dimensions = collections.defaultdict(lambda: 1)
_default_dx = 0.25
_default_method = 'deterministic'

#CRxD
_diffusion_d = None
_diffusion_a = None
_diffusion_b = None
_diffusion_p = None
_cur_node_indices = None
_diffusion_a_ptr, _diffusion_b_ptr, _diffusion_p_ptr = None, None, None

def set_solve_type(domain=None, dimension=None, dx=None, nsubseg=None, method=None):
    """Specify the numerical discretization and solver options.
    
    domain -- a section or Python iterable of sections"""
    setting_default = False
    if domain is None:
        domain = h.allsec()
        setting_default = True
    elif isinstance(domain, nrn.Section):
        domain = [domain]
    
    # NOTE: These attributes are set on a per-nrn.Section basis; they cannot 
    #       assume Section1D objects exist because they might be specified before
    #       those objects are created
    
    # domain is now always an iterable (or invalid)
    if method is not None:
        raise RxDException('using set_solve_type to specify method is not yet implemented')
    if dimension is not None:
        if dimension not in (1, 3):
            raise RxDException('invalid option to set_solve_type: dimension must be 1 or 3')
        factory = lambda: dimension
        if setting_default:
            _dimensions.default_factory = factory
        for sec in domain:
            _dimensions[sec] = dimension 
    if dx is not None:
        raise RxDException('using set_solve_type to specify dx is not yet implemented')
    if nsubseg is not None:
        raise RxDException('using set_solve_type to specify nsubseg is not yet implemented')
    

def _unregister_reaction(r):
    global _all_reactions
    for i, r2 in enumerate(_all_reactions):
        if r2() == r:
            del _all_reactions[i]
            break

def _register_reaction(r):
    # TODO: should we search to make sure that (a weakref to) r hasn't already been added?
    global _all_reactions, _external_solver_initialized
    _all_reactions.append(_weakref_ref(r))
    _external_solver_initialized = False
    
def _after_advance():
    global last_diam_change_cnt
    last_diam_change_cnt = _diam_change_count.value
    
def re_init():
    """reinitializes all rxd concentrations to match HOC values, updates matrices"""
    global _external_solver_initialized
    h.define_shape()
    
    if not species._has_3d:
        # TODO: if we do have 3D, make sure that we do the necessary parts of this
    
        # update current pointers
        section1d._purge_cptrs()
        for sr in list(_species_get_all_species().values()):
            s = sr()
            if s is not None:
                s._register_cptrs()
        
        # update matrix equations
        _setup_matrices()
    for sr in list(_species_get_all_species().values()):
        s = sr()
        if s is not None: s.re_init()
    # TODO: is this safe?        
    _cvode_object.re_init()

    _external_solver_initialized = False
    
def _invalidate_matrices():
    # TODO: make a separate variable for this?
    global _diffusion_matrix, _external_solver_initialized, last_structure_change_cnt
    _diffusion_matrix = None
    last_structure_change_cnt = None
    _external_solver_initialized = False

_rxd_offset = None

def _atolscale(y):
    real_index_lookup = {item: index for index, item in enumerate(_nonzero_volume_indices)}
    for sr in list(_species_get_all_species().values()):
        s = sr()
        if s is not None:
            shifted_i = [real_index_lookup[i] + _rxd_offset for i in s.indices() if i in real_index_lookup]
            y[shifted_i] *= s._atolscale

def _ode_count(offset):
    global _rxd_offset, last_structure_change_cnt, _structure_change_count
    initializer._do_init()
    _rxd_offset = offset - len(_nonzero_volume_indices)
    if _diffusion_matrix is None or last_structure_change_cnt != _structure_change_count.value: _setup_matrices()
    last_structure_change_cnt = _structure_change_count.value
    return len(_nonzero_volume_indices)

def _ode_reinit(y):
    y[_rxd_offset : _rxd_offset + len(_nonzero_volume_indices)] = _node_get_states()[_nonzero_volume_indices]

def _ode_fun(t, y, ydot):
    initializer.assert_initialized()
    lo = _rxd_offset
    hi = lo + len(_nonzero_volume_indices)
    if lo == hi: return
    states = _node_get_states().copy()
    states[_nonzero_volume_indices] = y[lo : hi]

    # need to fill in the zero volume states with the correct concentration
    # this assumes that states at the zero volume indices is zero (although that
    # assumption could be easily removed)
    #matrix = _scipy_sparse_dok_matrix((len(_zero_volume_indices), len(states)))
    """
    for i, row in enumerate(_zero_volume_indices):
        d = _diffusion_matrix[row, row]
        if d:
            nzj = _diffusion_matrix[row].nonzero()[1]
            print 'nzj:', nzj
            for j in nzj:
                matrix[i, j] = -_diffusion_matrix[row, j] / d
    states[_zero_volume_indices] = matrix * states
    """
    if len(_zero_volume_indices):
        states[_zero_volume_indices] = _mat_for_zero_volume_nodes * states
    """
    for i in _zero_volume_indices:
        v = _diffusion_matrix[i] * states
        d = _diffusion_matrix[i, i]
        if d:
            states[i] = -v / d
    """
    # TODO: make this so that the section1d parts use cptrs (can't do this directly for 3D because sum, but could maybe move that into the C)
    # the old way: _section1d_transfer_to_legacy()
#    for sr in _species_get_all_species().values():
#        s = sr()
#        if s is not None: s._transfer_to_legacy()

    
    if ydot is not None:
        # diffusion_matrix = - jacobian    
        ydot[lo : hi] = (_rxd_reaction(states) - _diffusion_matrix * states)[_nonzero_volume_indices]
        
    states[_zero_volume_indices] = 0

_rxd_induced_currents = None
_memb_cur_ptrs= []
def _setup_memb_currents():
    global _memb_cur_ptrs
    initializer._do_init()
    # setup membrane fluxes from our stuff
    # TODO: cache the memb_cur_ptrs, memb_cur_charges, memb_net_charges, memb_cur_mapped
    #       because won't change very often
    # need this; think it's because of initialization of mod files
    if _curr_indices is None: return
    SPECIES_ABSENT = -1
    # TODO: change so that this is only called when there are in fact currents
    rxd_memb_scales = []
    _memb_cur_ptrs = []
    memb_cur_charges = []
    memb_net_charges = []
    memb_cur_mapped = []
    memb_cur_mapped_ecs = []
    for rptr in _all_reactions:
        r = rptr()
        if r and r._membrane_flux:
            scales = r._memb_scales
            rxd_memb_scales.extend(scales)
            _memb_cur_ptrs += r._cur_ptrs
            memb_cur_mapped += r._cur_mapped
            memb_cur_mapped_ecs += r._cur_mapped_ecs
            memb_cur_charges += [r._cur_charges] * len(scales)
            memb_net_charges += [r._net_charges] * len(scales)
    ecs_map = [SPECIES_ABSENT if i is None else i for i in list(itertools.chain.from_iterable(itertools.chain.from_iterable(memb_cur_mapped_ecs)))]
    ics_map = [SPECIES_ABSENT if i is None else i for i in list(itertools.chain.from_iterable(itertools.chain.from_iterable(memb_cur_mapped)))]
    if _memb_cur_ptrs:
        cur_counts = [len(x) for x in memb_cur_mapped]
        num_currents = numpy.array(cur_counts).sum()
        setup_currents(len(_memb_cur_ptrs),
            num_currents,
            len(_curr_indices), # num_currents == len(_curr_indices) if no Extracellular
            _list_to_cint_array(cur_counts),
            _list_to_cint_array(memb_net_charges),
            _list_to_cint_array(_curr_indices),
            _list_to_cint_array(_cur_node_indices),
            _list_to_cdouble_array(rxd_memb_scales),
            _list_to_cint_array(list(itertools.chain.from_iterable(memb_cur_charges))),
            _list_to_pyobject_array(list(itertools.chain.from_iterable(_memb_cur_ptrs))),
            _list_to_cint_array(ics_map),
            _list_to_cint_array(ecs_map))
        
def _currents(rhs):
    return
    if rxd_memb_flux:
        # TODO: remove the asserts when this is verified to work
        assert(len(rxd_memb_flux) == len(_cur_node_indices))
        assert(len(rxd_memb_flux) == len(memb_cur_ptrs))
        assert(len(rxd_memb_flux) == len(memb_cur_charges))
        assert(len(rxd_memb_flux) == len(memb_net_charges))
        for flux, cur_ptrs, cur_charges, net_charge, i, cur_maps in zip(rxd_memb_flux, memb_cur_ptrs, memb_cur_charges, memb_net_charges, _cur_node_indices, memb_cur_mapped):
            rhs[i] -= net_charge * flux
            #import sys
            #sys.exit()
            # TODO: remove this assert when more thoroughly tested
            assert(len(cur_ptrs) == len(cur_maps))
            for ptr, charge, cur_map_i in zip(cur_ptrs, cur_charges, cur_maps):
                # this has the opposite sign of the above because positive
                # currents lower the membrane potential
                cur = charge * flux
                ptr[0] += cur
                for c in cur_map_i:
                    _rxd_induced_currents[c] += cur
                #for sign, c in zip([-1, 1], cur_maps):
                #    if c is not None:
                #        _rxd_induced_currents[c] += sign * cur

_last_m = None
_last_preconditioner = None
_fixed_step_count = 0


def _rxd_reaction(states):
    # TODO: this probably shouldn't be here
    # TODO: this was included in the 3d, probably shouldn't be there either
    # TODO: if its None and there is 3D... should we do anything special?
    if _diffusion_matrix is None and not species._has_3d: _setup_matrices()

    b = _numpy_zeros(len(states))
    
    
    if _curr_ptr_vector is not None:
        _curr_ptr_vector.gather(_curr_ptr_storage_nrn)
        b[_curr_indices] = _curr_scales * (_curr_ptr_storage - _rxd_induced_currents)   
    
    b[_curr_indices] = _curr_scales * [ptr[0] for ptr in _curr_ptrs]

    # TODO: store weak references to the r._evaluate in addition to r so no
    #       repeated lookups
    #for rptr in _all_reactions:
    #    r = rptr()
    #    if r:
    #        indices, mult, rate = r._evaluate(states)
    #        we split this in parts to allow for multiplicities and to allow stochastic to make the same changes in different places
    #        for i, m in zip(indices, mult):
    #            b[i] += m * rate

    node._apply_node_fluxes(b)
    return b
    
_last_preconditioner_dt = 0
_last_dt = None
_last_m = None
_diffusion_d = None
_diffusion_a = None
_diffusion_b = None
_diffusion_p = None
_cur_node_indices = None

_diffusion_a_ptr, _diffusion_b_ptr, _diffusion_p_ptr = None, None, None

def _setup():
    initializer._do_init()
    # TODO: this is when I should resetup matrices (structure changed event)
    global _last_dt, _external_solver_initialized
    _last_dt = None
    _external_solver_initialized = False
    
    # Using C-code for reactions
    options.use_reaction_contribution_to_jacobian = False

def _find_librxdmath():
    import glob
    base_path = os.path.join(h.neuronhome(), "..", "..", platform.machine(), "lib", "librxdmath")
    success = False 
    for extension in ['', '.dll', '.so', '.dylib']:
        dll = base_path  + extension
        try:
            success = os.path.exists(dll) 
        except:
            pass
        if success: break
    if not success:
        if sys.platform.lower().startswith("win"):
            dll = os.path.join(h.neuronhome(), 'bin', 'librxdmath.dll')
            success = os.path.exists(dll)
        if not success:
            raise RxDException('unable to connect to the librxdmath library')
    return dll
    
def _c_compile(formula):
    filename = 'rxddll' + str(uuid.uuid1())
    with open(filename + '.c', 'w') as f:
        f.write(formula)
    math_library = '-lm'
    fpic = '-fPIC'
    try:
        gcc = os.environ["CC"]
    except:
        #when running on windows try and used the gcc included with NEURON
        if sys.platform.lower().startswith("win"):
            math_library = ''
            fpic = ''
            gcc = os.path.join(h.neuronhome(),"mingw","mingw64","bin","x86_64-w64-mingw32-gcc.exe")
            if not os.path.isfile(gcc):
                raise RxDException("unable to locate a C compiler. Please `set CC=<path to C compiler>`")
        else:
            gcc = "gcc"
    #TODO: Check this works on non-Linux machines
    gcc_cmd =  "%s -I%s -I%s " % (gcc, sysconfig.get_python_inc(), os.path.join(h.neuronhome(), "..", "..", "include", "nrn"))
    gcc_cmd += "-shared %s  %s.c %s " % (fpic, filename, _find_librxdmath())
    gcc_cmd += "-o %s.so %s" % (filename, math_library)
    if sys.platform.lower().startswith("win"):
        my_path = os.getenv('PATH')
        os.putenv('PATH', my_path + ';' + os.path.join(h.neuronhome(),"mingw","mingw64","bin"))
        os.system(gcc_cmd)
        os.putenv('PATH', my_path)
    else:
        os.system(gcc_cmd)
    #TODO: Find a better way of letting the system locate librxdmath.so.0
    rxdmath_dll = ctypes.cdll[_find_librxdmath()]
    dll = ctypes.cdll['./%s.so' % filename]
    reaction = dll.reaction
    reaction.argtypes = [ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_double)] 
    reaction.restype = ctypes.c_double
    os.remove(filename + '.c')
    if sys.platform.lower().startswith("win"):
        #cannot remove dll that are in use
        _windows_dll.append(weakref.ref(dll))
        _windows_dll_files.append(filename + ".so")
    else:
        os.remove(filename + '.so')
    return reaction


def _conductance(d):
    pass
    
def _ode_jacobian(dt, t, ypred, fpred):
    #print '_ode_jacobian: dt = %g, last_dt = %r' % (dt, _last_dt)
    lo = _rxd_offset
    hi = lo + len(_nonzero_volume_indices)    
    _reaction_matrix_setup(dt, ypred[lo : hi])

_curr_ptr_vector = None
_curr_ptr_storage = None
_curr_ptr_storage_nrn = None
pinverse = None
_cur_map = None
_h_ptrvector = h.PtrVector
_h_vector = h.Vector

_structure_change_count = nrn_dll_sym('structure_change_cnt', _ctypes_c_int)
_diam_change_count = nrn_dll_sym('diam_change_cnt', _ctypes_c_int)

def _donothing(): pass

def _update_node_data(force=False):
    global last_diam_change_cnt, last_structure_change_cnt, _curr_indices, _curr_scales, _curr_ptrs, _cur_map
    global _curr_ptr_vector, _curr_ptr_storage, _curr_ptr_storage_nrn
    if last_diam_change_cnt != _diam_change_count.value or _structure_change_count.value != last_structure_change_cnt or force:
        _cur_map = {}
        last_diam_change_cnt = _diam_change_count.value
        last_structure_change_cnt = _structure_change_count.value
        #if not species._has_3d:
        # TODO: merge this with the 3d/hybrid case?
        nsegs_changed = 0
        for sr in list(_species_get_all_species().values()):
            s = sr()
            if s is not None: nsegs_changed += s._update_node_data()
        if nsegs_changed:
            section1d._purge_cptrs()
            for sr in list(_species_get_all_species().values()):
                s = sr()
                if s is not None:
                    s._update_region_indices(True)
                    s._register_cptrs()
            if species._has_1d and species._1d_submatrix_n():
                volumes = node._get_data()[0]
                _zero_volume_indices = (numpy.where(volumes == 0)[0]).astype(numpy.int_)
            setup_solver(_node_get_states(), len(_node_get_states()), _zero_volume_indices, len(_zero_volume_indices), h._ref_t, h._ref_dt)
            # TODO: separate compiling reactions -- so the indices can be updated without recompiling
            _compile_reactions()

        #end#if
        for rptr in _all_reactions:
            r = rptr()
            if r is not None: r._update_indices()
        _curr_indices = []
        _curr_scales = []
        _curr_ptrs = []
        for sr in list(_species_get_all_species().values()):
            s = sr()
            if s is not None: s._setup_currents(_curr_indices, _curr_scales, _curr_ptrs, _cur_map)
        
        num = len(_curr_ptrs)
        if num:
            _curr_ptr_vector = _h_ptrvector(num)
            _curr_ptr_vector.ptr_update_callback(_donothing)
            for i, ptr in enumerate(_curr_ptrs):
                _curr_ptr_vector.pset(i, ptr)
            
            _curr_ptr_storage_nrn = _h_vector(num)
            _curr_ptr_storage = _curr_ptr_storage_nrn.as_numpy()
        else:
            _curr_ptr_vector = None

        #_curr_scales = _numpy_array(_curr_scales)        


def _matrix_to_rxd_sparse(m):
    """precondition: assumes m a numpy array"""
    nonzero_i, nonzero_j = list(zip(*list(m.keys())))
    nonzero_values = numpy.ascontiguousarray(list(m.values()), dtype=numpy.float64)

    # number of rows
    n = m.shape[1]

    return n, len(nonzero_i), numpy.ascontiguousarray(nonzero_i, dtype=numpy.int_), numpy.ascontiguousarray(nonzero_j, dtype=numpy.int_), nonzero_values


_euler_matrix = None

# TODO: make sure this does the right thing when the diffusion constant changes between two neighboring nodes
def _setup_matrices():
    global _curr_ptrs
    global _cur_node_indices
    global _zero_volume_indices

    # TODO: this sometimes seems to get called twice. Figure out why and fix, if possible.

    # if the shape has changed update the nodes
    _update_node_data()

    n = len(_node_get_states())
    
    #TODO: Replace with ADI version 
    """
    if species._has_3d:
        _euler_matrix = _scipy_sparse_dok_matrix((n, n), dtype=float)

        for sr in list(_species_get_all_species().values()):
            s = sr()
            if s is not None: s._setup_matrices3d(_euler_matrix)

        _diffusion_matrix = -_euler_matrix

        _euler_matrix = _euler_matrix.tocsr()
        _update_node_data(True)

        # NOTE: if we also have 1D, this will be replaced with the correct values below
        _zero_volume_indices = []
        _nonzero_volume_indices = list(range(len(_node_get_states())))
        
    """
    if species._has_1d:
        n = species._1d_submatrix_n()
        # TODO: initialization is slow. track down why
        
        _last_dt = None
        
        for sr in list(_species_get_all_species().values()):
            s = sr()
            if s is not None:
                s._assign_parents()
        
        _update_node_data(True)

        volumes = node._get_data()[0]
        _zero_volume_indices = (numpy.where(volumes == 0)[0]).astype(numpy.int_)
        _nonzero_volume_indices = volumes.nonzero()[0]

        # remove old linearmodeladdition
        _linmodadd_cur = None
        
        if n:        
            # create sparse matrix for C in cy'+gy=b
            c_diagonal = numpy.zeros(n,dtype=ctypes.c_double)
            # most entries are 1 except those corresponding to the 0 and 1 ends
                        
            # create the matrix G
            #if not species._has_3d:
            #    # if we have both, then put the 1D stuff into the matrix that already exists for 3D
            _diffusion_matrix = [dict() for idx in range(n)]  
            for sr in list(_species_get_all_species().values()):
                s = sr()
                if s is not None:
                    s._setup_diffusion_matrix(_diffusion_matrix)
                    s._setup_c_matrix(c_diagonal)
                    #print '_diffusion_matrix.shape = %r, n = %r, species._has_3d = %r' % (_diffusion_matrix.shape, n, species._has_3d)
            euler_matrix_i, euler_matrix_j, euler_matrix_nonzero = [], [], []
            for i in range(n):
                mat_i = _diffusion_matrix[i]
                euler_matrix_i.extend(itertools.repeat(i,len(mat_i)))
                euler_matrix_j.extend(mat_i.keys())
                euler_matrix_nonzero.extend(mat_i.values())
            euler_matrix_nnonzero = len(euler_matrix_nonzero)
            assert(len(euler_matrix_i) == len(euler_matrix_j) == len(euler_matrix_nonzero))
            # modify C for cases where no diffusive coupling of 0, 1 ends
            # TODO: is there a better way to handle no diffusion?
            #for i in range(n):
            #    if not _diffusion_matrix[i, i]:
            #        _linmodadd_c[i, i] = 1

            
            # setup for induced membrane currents
            _cur_node_indices = []

            for rptr in _all_reactions:
                r = rptr()
                if r is not None:
                    r._setup_membrane_fluxes(_cur_node_indices, _cur_map)
                    
            #_cvode_object.re_init()    

            #if species._has_3d:
            #    _euler_matrix = -_diffusion_matrix


    #TODO: Replace this this to handle 1d/3d hybrid models
    """
    if species._has_1d and species._has_3d:
        # TODO: add connections to matrix; for now: find them
        hybrid_neighbors = collections.defaultdict(lambda: [])
        hybrid_diams = {}
        dxs = set()
        for sr in list(_species_get_all_species().values()):
            s = sr()
            if s is not None:
                if s._nodes and s._secs:
                    # have both 1D and 3D, so find the neighbors
                    # for each of the 3D sections, find the parent sections
                    for r in s._regions:
                        dxs.add(r._dx)
                        for sec in r._secs3d:
                            parent_seg = sec.trueparentseg()
                            parent_sec = None if not parent_seg else parent_seg.sec
                            # are any of these a match with a 1d section?
                            if s._has_region_section(r, parent_sec):
                                # this section has a 1d section that is a parent
                                index1d, indices3d = _get_node_indices(s, r, sec, h.section_orientation(sec=sec), parent_sec, h.parent_connection(sec=sec))
                                hybrid_neighbors[index1d] += indices3d
                                hybrid_diams[index1d] = parent_seg.diam
                            else:
                                for sec1d in r._secs1d:
                                    parent_1d_seg = sec1d.trueparentseg()
                                    parent_1d = None if not parent_seg else parent_seg.sec
                                    if parent_1d == sec:
                                        # it is the parent of a 1d section
                                        index1d, indices3d = _get_node_indices(s, r, sec, h.parent_connection(sec=sec1d), sec1d, sec1d.orientation())
                                        hybrid_neighbors[index1d] += indices3d
                                        hybrid_diams[index1d] = parent_1d_seg.diam
                                        break
                                    elif parent_1d == parent_sec:
                                        # it connects to the parent of a 1d section
                                        index1d, indices3d = _get_node_indices(s, r, sec, h.section_orientation(sec=sec), sec1d, sec1d.orientation())
                                        hybrid_neighbors[index1d] += indices3d
                                        hybrid_diams[index1d] = parent_1d_seg.diam
                                        break
        if len(dxs) > 1:
            raise RxDException('currently require a unique value for dx')
        dx = dxs.pop()
        diffs = node._diffs
        n = len(_node_get_states())
        # TODO: validate that we're doing the right thing at boundaries
        for index1d in list(hybrid_neighbors.keys()):
            neighbors3d = set(hybrid_neighbors[index1d])
            # NOTE: splitting the connection area equally across all the connecting nodes
            area = (numpy.pi * 0.25 * hybrid_diams[index1d] ** 2) / len(neighbors3d)
            for i in neighbors3d:
                d = diffs[i]
                vol = node._volumes[i]
                rate = d * area / (vol * dx / 2.)
                # make the connections on the 3d side
                _euler_matrix[i, i] -= rate
                _euler_matrix[i, index1d] += rate
                # make the connections on the 1d side (scale by vol because conserving mass not volume)
                _euler_matrix[index1d, index1d] -= rate * vol
                _euler_matrix[index1d, i] += rate * vol
            #print 'index1d row sum:', sum(_euler_matrix[index1d, j] for j in xrange(n))
            #print 'index1d col sum:', sum(_euler_matrix[j, index1d] for j in xrange(n))
    """
    #CRxD
    if n and euler_matrix_nnonzero > 0:
        _update_node_data()
        section1d._transfer_to_legacy()
        set_euler_matrix(n, euler_matrix_nnonzero,
                         _list_to_clong_array(euler_matrix_i),
                         _list_to_clong_array(euler_matrix_j),
                         _list_to_cdouble_array(euler_matrix_nonzero),
                         _zero_volume_indices,
                         len(_zero_volume_indices),
                         c_diagonal)
    else:
        rxd_set_no_diffusion()
        setup_solver(_node_get_states(), len(_node_get_states()), _zero_volume_indices, len(_zero_volume_indices), h._ref_t, h._ref_dt)
    
    if _curr_indices is not None and len(_curr_indices) > 0:
        rxd_setup_curr_ptrs(len(_curr_indices), _list_to_cint_array(_curr_indices),
            numpy.concatenate(_curr_scales), _list_to_pyobject_array(_curr_ptrs))

    if section1d._all_cindices is not None and len(section1d._all_cindices) > 0:
        rxd_setup_conc_ptrs(len(section1d._all_cindices), 
             _list_to_cint_array(section1d._all_cindices), 
             _list_to_pyobject_array(section1d._all_cptrs))

    # we do this last because of performance issues with changing sparsity of csr matrices
    """
    if _diffusion_matrix is not None:
        _diffusion_matrix = _diffusion_matrix.tocsr()
    if _euler_matrix is not None:
        _euler_matrix = _euler_matrix.tocsr()

    if species._has_1d:
        if species._has_3d:
            _diffusion_matrix = -_euler_matrix
        n = species._1d_submatrix_n()
        if n:
            matrix = _diffusion_matrix[_zero_volume_indices].tocsr()
            indptr = matrix.indptr
            matrixdata = matrix.data
            count = len(_zero_volume_indices)
            for row, i in enumerate(_zero_volume_indices):
                d = _diffusion_matrix[i, i]
                if d:
                    matrixdata[indptr[row] : indptr[row + 1]] /= -d
                    matrix[row, i] = 0
                else:
                    matrixdata[indptr[row] : indptr[row + 1]] = 0
            global _mat_for_zero_volume_nodes
            _mat_for_zero_volume_nodes = matrix
            # TODO: _mat_for_zero_volume_nodes is used for CVode.
            #       Figure out if/how it has to be changed for hybrid 1D/3D sims (probably just augment with identity? or change how its used to avoid multiplying by I)
    
    """
    
    """
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
"""                
                


def _get_node_indices(species, region, sec3d, x3d, sec1d, x1d):
    # TODO: remove need for this assumption
    assert(x1d in (0, 1))
    disc_indices = region._indices_from_sec_x(sec3d, x3d)
    #print '%r(%g) connects to the 1d section %r(%g)' % (sec3d, x3d, sec1d, x1d)
    #print 'disc indices: %r' % disc_indices
    indices3d = []
    for node in species._nodes:
        if node._r == region:
            for i, j, k in disc_indices:
                if node._i == i and node._j == j and node._k == k:
                    indices3d.append(node._index)
                    #print 'found node %d with coordinates (%g, %g, %g)' % (node._index, node.x3d, node.y3d, node.z3d)
    # discard duplicates...
    # TODO: really, need to figure out all the 3d nodes connecting to a given 1d endpoint, then unique that
    indices3d = list(set(indices3d))
    #print '3d matrix indices: %r' % indices3d
    # TODO: remove the need for this assertion
    if x1d == sec1d.orientation():
        # TODO: make this whole thing more efficient
        # the parent node is the nonzero index on the first row before the diagonal
        first_row = min([node._index for node in species.nodes(region)(sec1d)])
        for j in range(first_row):
            if _euler_matrix[first_row, j] != 0:
                index_1d = j
                break
        else:
            raise RxDException('should never get here; could not find parent')
    elif x1d == 1 - sec1d.orientation():
        # the ending zero-volume node is the one after the last node
        # TODO: make this more efficient
        index_1d = max([node._index for node in species.nodes(region)(sec1d)]) + 1
    else:
        raise RxDException('should never get here; _get_node_indices apparently only partly converted to allow connecting to 1d in middle')
    #print '1d index is %d' % index_1d
    
    return index_1d, indices3d

def _compile_reactions():
    #clear all previous reactions (intracellular & extracellular) and the
    #supporting indexes
    #_windows_remove_dlls()
    clear_rates()
    
    regions_inv = dict() #regions -> reactions that occur there
    species_by_region = dict()
    all_species_involed = set()
    location_count = 0
    
    ecs_regions_inv = dict()
    ecs_species_by_region = dict()
    ecs_all_species_involed = set()
    ecs_mc_species_involved = set()   
    from . import rate, multiCompartmentReaction

    #Find sets of sections that contain the same regions
    from .region import _c_region
    matched_regions = [] # the different combinations of regions that arise in different sections
    for nrnsec in list(section1d._rxd_sec_lookup.keys()):
        set_of_regions = set() # a set of the regions that occur in a given section
        for sec in section1d._rxd_sec_lookup[nrnsec]:
            if sec(): set_of_regions.add(sec()._region)
        if set_of_regions not in matched_regions:
            matched_regions.append(set_of_regions)
    region._c_region_lookup = dict()
    
    #create a c_region instance for each of the unique sets of regions
    c_region_list = []
    for sets in matched_regions:
        c_region_list.append(_c_region(sets))
    

    for rptr in _all_reactions:
        r = rptr()
        if not r:
            continue

        #Find all the species involved
        if isinstance(r,rate.Rate):
            if not r._species():
                continue
            sptrs = set(list(r._involved_species) + [r._species])
        else:
            sptrs  = set(list(r._involved_species) + r._dests + r._sources)
        
        #Find all the regions involved
        if isinstance(r, multiCompartmentReaction.MultiCompartmentReaction):
            if not hasattr(r._mult, 'flatten'):
                r._update_indices()
            react_regions = [s()._extracellular()._region for s in r._sources + r._dests if isinstance(s(),species.SpeciesOnExtracellular)] + [s()._region() for s in r._sources + r._dests if not isinstance(s(),species.SpeciesOnExtracellular)]
            react_regions +=  [sptr()._region() for sptr in sptrs if isinstance(sptr(),species.SpeciesOnRegion)]
        #if regions are specified - use those
        elif hasattr(r,'_active_regions'):
            react_regions = r._active_regions
        #Otherwise use all the regions where the species are
        else:
            react_regions = set()
            nsp = 0
            for sp in sptrs:
                s = sp()
                nsp += 1
                if isinstance(s,species.SpeciesOnRegion):
                    react_regions.add(s._region())
                elif isinstance(s,species.SpeciesOnExtracellular):
                    react_regions.add(s._extracellular()._region)
                elif isinstance(s,species._ExtracellularSpecies):
                    react_regions.add(s._region)
                elif None not in s._regions:
                    [react_regions.add(reg) for reg in s._regions + s._extracellular_regions]
            react_regions = list(react_regions)
            #Only regions where ALL the species are present -- unless it is a membrane
            #from collections import Counter
            #from . import geometry as geo
            #react_regions = [reg for reg, count in Counter(react_regions).iteritems() if count == nsp or isinstance(reg.geometry,geo.ScalableBorder)]
        #Any intracellular regions
        if not all([isinstance(x, region.Extracellular) for x in react_regions]):
            species_involved = []
            for sp in sptrs:
                s = sp()
                if not isinstance(s, species.SpeciesOnExtracellular):
                    all_species_involed.add(s)
                    species_involved.append(s)
            for reg in react_regions:
                if isinstance(reg, region.Extracellular):
                    continue
                if reg in regions_inv:
                    regions_inv[reg].append(rptr)
                else:
                    regions_inv[reg] = [rptr]
                if reg in species_by_region:
                    species_by_region[reg] = species_by_region[reg].union(species_involved)
                else:
                    species_by_region[reg] = set(species_involved)
                    for sec in reg._secs:
                        location_count += sec.nseg
        #Any extracellular regions
        if any([isinstance(x, region.Extracellular) for x in react_regions]):
            #MultiCompartment - so can have both extracellular and intracellular regions
            if isinstance(r, multiCompartmentReaction.MultiCompartmentReaction):
                for sp in sptrs:
                    s = sp()
                    if isinstance(s,species._ExtracellularSpecies):
                        ecs_mc_species_involved.add(s)
                    elif isinstance(s,species.SpeciesOnExtracellular):
                        ecs_mc_species_involved.add(s._extracellular())
                for reg in react_regions:
                    if reg in list(ecs_species_by_region.keys()):
                        ecs_species_by_region[reg] = ecs_species_by_region[reg].union(ecs_mc_species_involved)
                    else:
                        ecs_species_by_region[reg] = set(ecs_mc_species_involved)
            #Otherwise - reaction can only have extracellular regions
            else:
                ecs_species_involved = []
                for sp in sptrs:
                    s = sp()
                    ecs_all_species_involed.add(s)
                    ecs_species_involved.append(s)
                if any([isinstance(x, region.Region) for x in react_regions]):
                    raise RxDException("Error: an %s cannot have both Extracellular and Intracellular regions. Use a MultiCompartmentReaction or specify the desired region with the 'region=' keyword argument", rptr().__class__)
                for reg in react_regions:
                    if not isinstance(reg, region.Extracellular):
                        continue

                    if reg in ecs_regions_inv:
                        ecs_regions_inv[reg].append(rptr)
                    else:
                        ecs_regions_inv[reg] = [rptr]
                    if reg in ecs_species_by_region:
                        ecs_species_by_region[reg] = ecs_species_by_region[reg].union(ecs_species_involved)
                    else:
                        ecs_species_by_region[reg] = set(ecs_species_involved)
    #Create lists of indexes for intracellular reactions and rates
    nseg_by_region = []     # a list of the number of segments for each region
    # a table for location,species -> state index
    location_index = []
    for reg in regions_inv:
        rptr = weakref.ref(reg)
        for c_region in region._c_region_lookup[rptr]:
            for react in regions_inv[reg]:
                c_region.add_reaction(react,rptr)
                c_region.add_species(species_by_region[reg])
                if reg in ecs_species_by_region:
                    c_region.add_ecs_species(ecs_species_by_region[reg])

    # now setup the reactions
    setup_solver(_node_get_states(), len(_node_get_states()), _zero_volume_indices, len(_zero_volume_indices), h._ref_t, h._ref_dt)
    #if there are no reactions
    if location_count == 0 and len(ecs_regions_inv) == 0:
        return None
    
    #Setup intracellular and multicompartment reactions
    if location_count > 0:
        from . import rate, multiCompartmentReaction
        for creg in c_region_list:
            creg._initalize()
            mc_mult_count = 0
            mc_mult_list = []
            species_ids_used = numpy.zeros((creg.num_species,creg.num_regions),bool)
            flux_ids_used = numpy.zeros((creg.num_species,creg.num_regions),bool)
            ecs_species_ids_used = numpy.zeros((creg.num_ecs_species,creg.num_regions),bool)
            fxn_string = _c_headers 
            fxn_string += 'void reaction(double** species, double** rhs, double* mult, double** species_ecs, double** rhs_ecs, double** flux)\n{'
            # declare the "rate" variable if any reactions (non-rates)
            for rprt in list(creg._react_regions.keys()):
                if not isinstance(rprt(),rate.Rate):
                    fxn_string += '\n\tdouble rate;'
                    break
            for rptr in  list(creg._react_regions.keys()):
                r = rptr()
                if isinstance(r,rate.Rate):
                    s = r._species()
                    species_id = creg._species_ids.get(s._id)
                    if isinstance(s,species.SpeciesOnRegion):
                        region_ids = [creg._region_ids.get(s._region()._id)]
                    else:
                        region_ids = creg._react_regions[rptr]
                    for region_id in region_ids:
                        rate_str = re.sub(r'species\[(\d+)\]\[(\d+)\]',lambda m: "species[%i][%i]" %  (creg._species_ids.get(int(m.groups()[0])), creg._region_ids.get(int(m.groups()[1]))), r._rate)
                        rate_str = re.sub(r'species\[(\d+)\]\[\]',lambda m: "species[%i][%i]" %  (creg._species_ids.get(int(m.groups()[0])), region_id), rate_str)
                        operator = '+=' if species_ids_used[species_id][region_id] else '='
                        fxn_string += "\n\trhs[%d][%d] %s %s;" % (species_id, region_id, operator, rate_str)
                        species_ids_used[species_id][region_id] = True
                elif isinstance(r, multiCompartmentReaction.MultiCompartmentReaction):
                    #Lookup the region_id for the reaction
                    for sptr in r._sources + r._dests:
                        if isinstance(sptr(),species.SpeciesOnExtracellular):
                            continue
                        region_id = creg._region_ids.get(sptr()._region()._id)
                    rate_str = re.sub(r'species\[(\d+)\]\[(\d+)\]',lambda m: "species[%i][%i]" %  (creg._species_ids.get(int(m.groups()[0])), creg._region_ids.get(int(m.groups()[1]))), r._rate)
                    rate_str = re.sub(r'species\[(\d+)\]\[\]',lambda m: "species[%i][%i]" %  (creg._species_ids.get(int(m.groups()[0])), region_id), rate_str)
                    rate_str = re.sub(r'species_ecs\[(\d+)\]',lambda m: "species_ecs[%i][%i]" %  (int(m.groups()[0]), region_id), rate_str)
    
                    fxn_string += "\n\trate = %s;" % rate_str
    
                    for sptr in r._sources + r._dests:
                        s = sptr()
                        if isinstance(s,species.SpeciesOnExtracellular):
                            species_id = s._extracellular()._grid_id
                            operator = '+=' if ecs_species_ids_used[species_id][region_id] else '='
                            fxn_string += "\n\trhs_ecs[%d][%d] %s mult[%d] * rate;" % (species_id, region_id, operator, mc_mult_count)
                            ecs_species_ids_used[species_id][region_id] = True
                        else:
                            species_id = creg._species_ids.get(s._id)
                            region_id = creg._region_ids.get(sptr()._region()._id)
                            operator = '+=' if species_ids_used[species_id][region_id] else '='
                            fxn_string += "\n\trhs[%d][%d] %s mult[%d] * rate;" % (species_id, region_id, operator, mc_mult_count)
                            species_ids_used[species_id][region_id] = True
                            if r._membrane_flux:
                                operator = '+=' if flux_ids_used[species_id][region_id] else '='
                                fxn_string += "\n\tif(flux) flux[%d][%d] %s rate;" % (species_id, region_id, operator)
                                flux_ids_used[species_id][region_id] = True
                        #TODO: Fix problem if the whole region isn't part of the same aggregate c_region
                        mc_mult_count += 1
                    mc_mult_list.extend(r._mult.flatten())
                else:
                    for region_id in creg._react_regions[rptr]:
                        
                        rate_str = re.sub(r'species\[(\d+)\]\[(\d+)\]',lambda m: "species[%i][%i]" %  (creg._species_ids.get(int(m.groups()[0])), creg._region_ids.get(int(m.groups()[1]))), r._rate)
                        rate_str = re.sub(r'species\[(\d+)\]\[\]',lambda m: "species[%i][%i]" %  (creg._species_ids.get(int(m.groups()[0])), region_id), rate_str)
                        fxn_string += "\n\trate = %s;" % rate_str
                        summed_mults = collections.defaultdict(lambda: 0)
                        for (mult, sp) in zip(r._mult, r._sources + r._dests):
                            summed_mults[creg._species_ids.get(sp()._id)] += mult
                        for idx in sorted(summed_mults.keys()):
                            operator = '+=' if species_ids_used[idx][region_id] else '='
                            species_ids_used[idx][region_id] = True
                            fxn_string += "\n\trhs[%d][%d] %s (%g) * rate;" % (idx, region_id, operator, summed_mults[idx])
              
            fxn_string += "\n}\n"
            register_rate(creg.num_species, creg.num_regions, creg.num_segments, creg.get_state_index(),
                          creg.num_ecs_species, creg.get_ecs_species_ids(), creg.get_ecs_index(),
                          mc_mult_count, numpy.array(mc_mult_list, dtype=ctypes.c_double),
                          _c_compile(fxn_string))

    
    #Setup extracellular reactions
    if len(ecs_regions_inv) > 0:
        grid_ids = []
        all_gids = set() 
        fxn_string = _c_headers 
        #TODO: find the nrn include path in python
        #It is necessary for a couple of function in python that are not in math.h
        fxn_string += 'void reaction(double* species_ecs, double* rhs)\n{'
        # declare the "rate" variable if any reactions (non-rates)
        for rptr in [r for rlist in list(ecs_regions_inv.values()) for r in rlist]:
            if not isinstance(rptr(),rate.Rate):
                fxn_string += '\n\tdouble rate;'
                break
        #get a list of all grid_ids invovled
        for rptr in [r for rlist in list(ecs_regions_inv.values()) for r in rlist]:
            if isinstance(rptr(),rate.Rate):
                for sp in [rptr()._species] + rptr()._involved_species_ecs:
                    s = sp()[reg]._extracellular() if isinstance(sp(), species.Species) else sp()
                    all_gids.add(sp()._extracellular()._grid_id if isinstance(s, species.SpeciesOnExtracellular) else s._grid_id)
            else:
                for sp in rptr()._sources + rptr()._dests + rptr()._involved_species_ecs:
                    s = sp()[reg]._extracellular() if isinstance(sp(), species.Species) else sp()
                    all_gids.add(sp()._extracellular()._grid_id if isinstance(s, species.SpeciesOnExtracellular) else s._grid_id)
        all_gids = list(all_gids)
        for reg in ecs_regions_inv:
            for rptr in ecs_regions_inv[reg]:
                r = rptr()
                rate_str = re.sub(r'species_ecs\[(\d+)\]',lambda m: "species_ecs[%i]" %  [pid for pid,gid in enumerate(all_gids) if gid == int(m.groups()[0])][0], r._rate_ecs)
                if isinstance(r,rate.Rate):
                    s = r._species()
                    #Get underlying rxd._ExtracellularSpecies for the grid_id
                    if isinstance(s, species.Species):
                        s = s[reg]._extracellular()
                    elif isinstance(s, species.SpeciesOnExtracellular):
                        s = s._extracellular()
                    if s._grid_id in grid_ids:
                        operator = '+=' 
                    else:
                        operator = '='
                        grid_ids.append(s._grid_id)
                    pid = [pid for pid,gid in enumerate(all_gids) if gid == s._grid_id][0]
                    fxn_string += "\n\trhs[%d] %s %s;" % (pid, operator, rate_str)
                else:
                    idx=0
                    fxn_string += "\n\trate = %s;" %  rate_str
                    for sp in r._sources + r._dests:
                        s = sp()
                        #Get underlying rxd._ExtracellularSpecies for the grid_id
                        if isinstance(s, species.Species):
                            s = s[reg]._extracellular()
                        elif isinstance(s, species.SpeciesOnExtracellular):
                            s = s._extracellular()
                        if s._grid_id in grid_ids:
                            operator = '+=' 
                        else:
                            operator = '='
                            grid_ids.append(s._grid_id)
                        pid = [pid for pid,gid in enumerate(all_gids) if gid == s._grid_id][0]
                        fxn_string += "\n\trhs[%d] %s (%s)*rate;" % (pid, operator, r._mult[idx])
                        idx += 1
        fxn_string += "\n}\n"
        ecs_register_reaction(0, len(all_gids), _list_to_cint_array(all_gids), _c_compile(fxn_string))

def _init():
    if len(species._all_species) == 0:
        return None
    initializer._do_init()
    # TODO: check about the 0<x<1 problem alluded to in the documentation
    h.define_shape()

    # if the shape has changed update the nodes
    _update_node_data()
    
    if species._has_1d:
        section1d._purge_cptrs()
    
    for sr in list(_species_get_all_species().values()):
        s = sr()
        if s is not None:
            # TODO: are there issues with hybrid or 3D here? (I don't think so, but here's a bookmark just in case)
            s._register_cptrs()
            s._finitialize()
    _setup_matrices()
    _compile_reactions()
    _setup_memb_currents()

def _init_concentration():
    if len(species._all_species) == 0:
        return None
    for sr in list(_species_get_all_species().values()):
        s = sr()
        if s is not None:
            # TODO: are there issues with hybrid or 3D here? (I don't think so, but here's a bookmark just in case)
            s._finitialize()



_has_nbs_registered = False
_nbs = None
do_setup_matrices_fptr = None
def _do_nbs_register():
    global _has_nbs_registered, _nbs, _fih, _fih2, _fih3, do_setup_matrices_fptr
    
    if not _has_nbs_registered:
        #from neuron import nonvint_block_supervisor as _nbs

        _has_nbs_registered = True
        #_nbs.register(_callbacks)  not used by crxd
    
        #
        # register the initialization handler and the ion register handler
        #
        _fih = h.FInitializeHandler(_init_concentration)
        _fih3 = h.FInitializeHandler(3, _init)

        set_setup_matrices = nrn_dll_sym('set_setup_matrices')
        set_setup_matrices.argtypes = [fptr_prototype]
        do_setup_matrices_fptr = fptr_prototype(_setup_matrices)
        set_setup_matrices(do_setup_matrices_fptr)

        _fih2 = h.FInitializeHandler(3, initializer._do_ion_register)


        #
        # register scatter/gather mechanisms
        #
        _cvode_object.extra_scatter_gather(0, _after_advance)
        

# register the Python callbacks
do_setup_fptr = fptr_prototype(_setup)
do_initialize_fptr = fptr_prototype(_init)
set_setup(do_setup_fptr)
set_initialize(do_initialize_fptr)

def _windows_remove_dlls():
    global _windows_dll_files, _windows_dll
    for (dll_ptr,filepath) in zip(_windows_dll,_windows_dll_files):
        dll = dll_ptr()
        if dll:
            handle = dll._handle
            del dll
            ctypes.windll.kernel32.FreeLibrary(handle)
        os.remove(filepath)
    _windows_dll_files = []
    _windows_dll = []
        
        
def nthread(n=None):
    if(n):
        _set_num_threads(n)
    return _get_num_threads()

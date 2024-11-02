from neuron import h, nrn, nrn_dll_sym
from . import species, node, section1d, region, generalizedReaction, constants
from .node import _point_indices
import weakref
import numpy
import ctypes
import atexit
from . import options
from .rxdException import RxDException
from . import initializer
import collections
import os
import sysconfig
import uuid
import sys
import itertools
from numpy.ctypeslib import ndpointer
import re
import platform
from warnings import warn

# aliases to avoid repeatedly doing multiple hash-table lookups
_species_get_all_species = species._get_all_species
_node_get_states = node._get_states
_ctypes_c_int = ctypes.c_int
_weakref_ref = weakref.ref

_external_solver = None
_external_solver_initialized = False
_windows_dll_files = []
_windows_dll = []


make_time_ptr = nrn_dll_sym("make_time_ptr")
make_time_ptr.argtypes = [ctypes.py_object, ctypes.py_object]
make_time_ptr(h._ref_dt, h._ref_t)

_double_ptr = ctypes.POINTER(ctypes.c_double)
_int_ptr = ctypes.POINTER(_ctypes_c_int)
_long_ptr = ctypes.POINTER(ctypes.c_long)


fptr_prototype = ctypes.CFUNCTYPE(None)
set_nonvint_block = nrn_dll_sym("set_nonvint_block")
set_nonvint_block(nrn_dll_sym("rxd_nonvint_block"))

set_setup = nrn_dll_sym("set_setup")
set_setup.argtypes = [fptr_prototype]
set_initialize = nrn_dll_sym("set_initialize")
set_initialize.argtypes = [fptr_prototype]

scatter_concentrations = nrn_dll_sym("scatter_concentrations")

# Transfer extracellular concentrations to NEURON
_fih_transfer_ecs = h.FInitializeHandler(1, scatter_concentrations)


rxd_set_no_diffusion = nrn_dll_sym("rxd_set_no_diffusion")

setup_solver = nrn_dll_sym("setup_solver")
setup_solver.argtypes = [
    ndpointer(ctypes.c_double),
    ctypes.c_int,
    numpy.ctypeslib.ndpointer(ctypes.c_long, flags="contiguous"),
    ctypes.c_int,
]

# states = None
_set_num_threads = nrn_dll_sym("set_num_threads")
_set_num_threads.argtypes = [ctypes.c_int]
_get_num_threads = nrn_dll_sym("get_num_threads")
_get_num_threads.restype = ctypes.c_int

free_conc_ptrs = nrn_dll_sym("free_conc_ptrs")
free_curr_ptrs = nrn_dll_sym("free_curr_ptrs")
clear_rates = nrn_dll_sym("clear_rates")
register_rate = nrn_dll_sym("register_rate")
register_rate.argtypes = [
    ctypes.c_int,  # num species
    ctypes.c_int,  # num parameters
    ctypes.c_int,  # num regions
    ctypes.c_int,  # num seg
    numpy.ctypeslib.ndpointer(ctypes.c_int, flags="contiguous"),  # species ids
    ctypes.c_int,  # num ecs species
    ctypes.c_int,  # num ecs parameters
    numpy.ctypeslib.ndpointer(ctypes.c_int, flags="contiguous"),  # ecs species ids
    numpy.ctypeslib.ndpointer(ctypes.c_int, flags="contiguous"),  # ecs indices
    ctypes.c_int,  # num multicompartment reactions
    numpy.ctypeslib.ndpointer(
        ctypes.c_double, flags="contiguous"
    ),  # multicompartment multipliers
    ctypes.POINTER(ctypes.py_object),  # voltage pointers
    ctypes._CFuncPtr,
]  # Reaction rate function

setup_currents = nrn_dll_sym("setup_currents")
setup_currents.argtypes = [
    ctypes.c_int,  # number of membrane currents
    ctypes.c_int,  # number induced currents
    _int_ptr,  # number of species involved in each membrane current
    _int_ptr,  # node indices
    _double_ptr,  # scaling (areas) of the fluxes
    ctypes.POINTER(ctypes.py_object),  # hoc pointers
    _int_ptr,  # maps for membrane fluxes
    _int_ptr,  # maps for ecs fluxes
]


ics_register_reaction = nrn_dll_sym("ics_register_reaction")
ics_register_reaction.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    _int_ptr,
    numpy.ctypeslib.ndpointer(dtype=numpy.uint64),
    ctypes.c_int,
    numpy.ctypeslib.ndpointer(dtype=float),
    ctypes._CFuncPtr,
]

ecs_register_reaction = nrn_dll_sym("ecs_register_reaction")
ecs_register_reaction.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    _int_ptr,
    ctypes._CFuncPtr,
]


set_hybrid_data = nrn_dll_sym("set_hybrid_data")
set_hybrid_data.argtypes = [
    numpy.ctypeslib.ndpointer(dtype=numpy.int64),
    numpy.ctypeslib.ndpointer(dtype=numpy.int64),
    numpy.ctypeslib.ndpointer(dtype=numpy.int64),
    numpy.ctypeslib.ndpointer(dtype=numpy.int64),
    numpy.ctypeslib.ndpointer(dtype=numpy.int64),
    numpy.ctypeslib.ndpointer(dtype=numpy.int64),
    numpy.ctypeslib.ndpointer(dtype=float),
    numpy.ctypeslib.ndpointer(dtype=float),
    numpy.ctypeslib.ndpointer(dtype=float),
    numpy.ctypeslib.ndpointer(dtype=float),
]

# ics_register_reaction = nrn_dll_sym('ics_register_reaction')
# ics_register_reaction.argtype = [ctypes.c_int, ctypes.c_int, _int_ptr, fptr_prototype]

set_euler_matrix = nrn_dll_sym("rxd_set_euler_matrix")
set_euler_matrix.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    _long_ptr,
    _long_ptr,
    _double_ptr,
    numpy.ctypeslib.ndpointer(numpy.double, flags="contiguous"),
]
rxd_setup_curr_ptrs = nrn_dll_sym("rxd_setup_curr_ptrs")
rxd_setup_curr_ptrs.argtypes = [
    ctypes.c_int,
    _int_ptr,
    numpy.ctypeslib.ndpointer(numpy.double, flags="contiguous"),
    ctypes.POINTER(ctypes.py_object),
]

rxd_setup_conc_ptrs = nrn_dll_sym("rxd_setup_conc_ptrs")
rxd_setup_conc_ptrs.argtypes = [
    ctypes.c_int,
    _int_ptr,
    ctypes.POINTER(ctypes.py_object),
]

rxd_include_node_flux1D = nrn_dll_sym("rxd_include_node_flux1D")
rxd_include_node_flux1D.argtypes = [
    ctypes.c_int,
    _long_ptr,
    _double_ptr,
    ctypes.POINTER(ctypes.py_object),
]

rxd_include_node_flux3D = nrn_dll_sym("rxd_include_node_flux3D")
rxd_include_node_flux3D.argtypes = [
    ctypes.c_int,
    _int_ptr,
    _int_ptr,
    _long_ptr,
    _double_ptr,
    ctypes.POINTER(ctypes.py_object),
]

_c_headers = """#include <math.h>
/*Some functions supported by numpy that aren't included in math.h
 * names and arguments match the wrappers used in rxdmath.py
 */
extern "C" {
double factorial(const double);
double degrees(const double);
void radians(const double, double*);
double log1p(const double);
double vtrap(const double, const double);
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
    # do not call __del__ that rearrange memory for states
    species.Species.__del__ = lambda x: None
    species._ExtracellularSpecies.__del__ = lambda x: None
    species._IntracellularSpecies.__del__ = lambda x: None
    section1d.Section1D.__del__ = lambda x: None
    generalizedReaction.GeneralizedReaction.__del__ = lambda x: None

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

_cvode_object = h.CVode()

last_diam_change_cnt = None
last_structure_change_cnt = None

_all_reactions = []

nrn_tree_solve = nrn_dll_sym("nrn_tree_solve")
nrn_tree_solve.restype = None

_dimensions = {1: h.SectionList(), 3: h.SectionList()}
_dimensions_default = 1
_default_dx = 0.25
_default_method = "deterministic"

# CRxD
_diffusion_d = None
_diffusion_a = None
_diffusion_b = None
_diffusion_p = None
_diffusion_a_ptr, _diffusion_b_ptr, _diffusion_p_ptr = None, None, None


def _domain_lookup(sec, dim=None):
    for d, sl in _dimensions.items():
        if sec in sl:
            if dim is not None and d != dim:
                sl.remove(sec)
                return _domain_lookup(sec, dim)
            return d
    dimension = dim if dim else _dimensions_default
    _dimensions[dimension].append(sec)
    return dimension


def set_solve_type(domain=None, dimension=None, dx=None, nsubseg=None, method=None):
    """Specify the numerical discretization and solver options.

    domain -- a section or Python iterable of sections"""

    global _dimensions_default, _dimensions

    if initializer.is_initialized():
        raise RxDException("set_solve_type must be called before any access to nodes.")

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
        raise RxDException(
            "using set_solve_type to specify method is not yet implemented"
        )
    if dimension is not None:
        if dimension not in (1, 3):
            raise RxDException(
                "invalid option to set_solve_type: dimension must be 1 or 3"
            )
        if setting_default:
            _dimensions_default = dimension
        for sec in domain:
            _domain_lookup(sec, dimension)
    if dx is not None:
        raise RxDException("using set_solve_type to specify dx is not yet implemented")
    if nsubseg is not None:
        raise RxDException(
            "using set_solve_type to specify nsubseg is not yet implemented"
        )


def _unregister_reaction(r):
    global _all_reactions
    react = r() if isinstance(r, weakref.ref) else r
    with initializer._init_lock:
        _all_reactions = list(
            filter(lambda x: x() is not None and x() != react, _all_reactions)
        )


def _register_reaction(r):
    # TODO: should we search to make sure that (a weakref to) r hasn't already been added?
    global _all_reactions, _external_solver_initialized
    with initializer._init_lock:
        _all_reactions.append(_weakref_ref(r))
        _external_solver_initialized = False


def _after_advance():
    global last_diam_change_cnt
    last_diam_change_cnt = _diam_change_count.value


def re_init():
    """reinitializes all rxd concentrations to match HOC values, updates matrices"""

    global _external_solver_initialized
    h.define_shape()

    section1d._purge_cptrs()
    for sr in _species_get_all_species():
        s = sr()
        if s is not None:
            s._register_cptrs()

    for sr in _species_get_all_species():
        s = sr()
        if s is not None:
            s.re_init()

    # update matrix equations
    _setup_matrices()

    if _cvode_object.active():
        _cvode_object.re_init()

    _external_solver_initialized = False


def _setup_memb_currents():
    initializer._do_init()
    # setup membrane fluxes from our stuff
    # TODO: cache the memb_cur_ptrs, memb_cur_charges, memb_net_charges, memb_cur_mapped
    #       because won't change very often
    # need this; think it's because of initialization of mod files
    # setup for induced membrane currents
    cur_node_indices = []
    cur_map = {}
    curr_indices = []
    curr_scales = []
    curr_ptrs = []
    for sr in _species_get_all_species():
        s = sr()
        if s is not None:
            s._setup_currents(curr_indices, curr_scales, curr_ptrs, cur_map)
        num = len(curr_ptrs)
        if num:
            curr_ptr_vector = _h_ptrvector(num)
            for i, ptr in enumerate(curr_ptrs):
                curr_ptr_vector.pset(i, ptr)
                curr_ptr_storage_nrn = _h_vector(num)
        else:
            curr_ptr_vector = None
            curr_ptr_storage_nrn = None
    for rptr in _all_reactions:
        r = rptr()
        if r is not None:
            r._update_indices()
            r._setup_membrane_fluxes(cur_node_indices, cur_map)
    if not curr_indices:
        free_curr_ptrs()
        return
    rxd_setup_curr_ptrs(
        len(curr_indices),
        _list_to_cint_array(curr_indices),
        numpy.concatenate(curr_scales),
        _list_to_pyobject_array(curr_ptrs),
    )

    SPECIES_ABSENT = -1
    # TODO: change so that this is only called when there are in fact currents
    rxd_memb_scales = []
    memb_cur_ptrs = []
    memb_cur_mapped = []
    memb_cur_mapped_ecs = []
    memb_cur_ptrs = []
    for rptr in _all_reactions:
        r = rptr()
        if r and r._membrane_flux:
            r._do_memb_scales(cur_map)
            scales = r._memb_scales
            rxd_memb_scales.extend(scales)
            memb_cur_ptrs += r._cur_ptrs
            memb_cur_mapped += r._cur_mapped
            memb_cur_mapped_ecs += r._cur_mapped_ecs
    ecs_map = [
        SPECIES_ABSENT if i is None else i
        for i in list(
            itertools.chain.from_iterable(
                itertools.chain.from_iterable(memb_cur_mapped_ecs)
            )
        )
    ]
    ics_map = [
        SPECIES_ABSENT if i is None else i
        for i in list(
            itertools.chain.from_iterable(
                itertools.chain.from_iterable(memb_cur_mapped)
            )
        )
    ]
    if memb_cur_ptrs:
        cur_counts = [
            len(x) for x in memb_cur_mapped
        ]  # TODO: is len(x) the same for all x?
        num_fluxes = numpy.array(cur_counts).sum()
        num_currents = len(memb_cur_ptrs)
        memb_cur_ptrs = list(itertools.chain.from_iterable(memb_cur_ptrs))
        setup_currents(
            num_currents,
            num_fluxes,
            _list_to_cint_array(cur_counts),
            _list_to_cint_array(cur_node_indices),
            _list_to_cdouble_array(rxd_memb_scales),
            _list_to_pyobject_array(memb_cur_ptrs),
            _list_to_cint_array(ics_map),
            _list_to_cint_array(ecs_map),
        )


def _setup():
    from . import initializer

    if not initializer.is_initialized():
        initializer._do_init()
    # TODO: this is when I should resetup matrices (structure changed event)
    global _external_solver_initialized, last_diam_change_cnt, last_structure_change_cnt
    _external_solver_initialized = False

    # Using C-code for reactions
    options.use_reaction_contribution_to_jacobian = False
    with initializer._init_lock:
        _update_node_data()


def _find_librxdmath():
    # cmake doesn't create x86_64 directory under install prefix
    base_path = os.path.join(h.neuronhome(), "..", "..", platform.machine())
    if not os.path.exists(base_path):
        base_path = os.path.join(h.neuronhome(), "..", "..")
    base_path = os.path.join(base_path, "lib", "librxdmath")
    success = False
    for extension in ["", ".dll", ".so", ".dylib"]:
        dll = base_path + extension
        try:
            success = os.path.exists(dll)
        except:
            pass
        if success:
            break
    if not success:
        if sys.platform.lower().startswith("win"):
            dll = os.path.join(h.neuronhome(), "bin", "librxdmath.dll")
            success = os.path.exists(dll)
        if not success:
            raise RxDException("unable to connect to the librxdmath library")
    return dll


def _cxx_compile(formula):
    filename = "rxddll" + str(uuid.uuid1())
    with open(filename + ".cpp", "w") as f:
        f.write(formula)
    math_library = "-lm"
    fpic = "-fPIC"
    try:
        gcc = os.environ["CXX"]
    except:
        # when running on windows try and used the gcc included with NEURON
        if sys.platform.lower().startswith("win"):
            math_library = ""
            fpic = ""
            gcc = os.path.join(
                h.neuronhome(), "mingw", "mingw64", "bin", "x86_64-w64-mingw32-g++.exe"
            )
            if not os.path.isfile(gcc):
                raise RxDException(
                    "unable to locate a CXX compiler. Please `set CXX=<path to CXX compiler>`"
                )
        else:
            gcc = "g++"
    # TODO: Check this works on non-Linux machines
    gcc_cmd = "%s -I%s " % (
        gcc,
        sysconfig.get_path("include"),
    )
    gcc_cmd += f"-shared {fpic} {filename}.cpp {_find_librxdmath()}"
    gcc_cmd += f" -o {filename}.so {math_library}"
    if sys.platform.lower().startswith("win"):
        my_path = os.getenv("PATH")
        os.putenv(
            "PATH",
            my_path + ";" + os.path.join(h.neuronhome(), "mingw", "mingw64", "bin"),
        )
        os.system(gcc_cmd)
        os.putenv("PATH", my_path)
    else:
        os.system(gcc_cmd)
    # the rxdmath_dll appears necessary for using librxdmath under certain gcc/OS pairs
    rxdmath_dll = ctypes.cdll[_find_librxdmath()]
    dll = ctypes.cdll[f"{os.path.abspath(filename)}.so"]
    reaction = dll.reaction
    reaction.argtypes = [
        ctypes.POINTER(ctypes.c_double),
        ctypes.POINTER(ctypes.c_double),
    ]
    reaction.restype = ctypes.c_double
    os.remove(f"{filename}.cpp")
    if sys.platform.lower().startswith("win"):
        # cannot remove dll that are in use
        _windows_dll.append(weakref.ref(dll))
        _windows_dll_files.append(f"{filename}.so")
    else:
        os.remove(f"{filename}.so")
    return reaction


_h_ptrvector = h.PtrVector
_h_vector = h.Vector

_structure_change_count = nrn_dll_sym("structure_change_cnt", _ctypes_c_int)
_diam_change_count = nrn_dll_sym("diam_change_cnt", _ctypes_c_int)


def _setup_units(force=False):
    if initializer.is_initialized():
        if force:
            clear_rates()
            _setup_memb_currents()
            _compile_reactions()
            if _cvode_object.active():
                _cvode_object.re_init()


def _update_node_data(force=False, newspecies=False):
    global last_diam_change_cnt, last_structure_change_cnt
    if (
        last_diam_change_cnt != _diam_change_count.value
        or _structure_change_count.value != last_structure_change_cnt
        or force
    ):
        last_diam_change_cnt = _diam_change_count.value
        last_structure_change_cnt = _structure_change_count.value
        # if not species._has_3d:
        # TODO: merge this with the 3d/hybrid case?
        if initializer.is_initialized():
            nsegs_changed = 0
            for sr in _species_get_all_species():
                s = sr()
                if s is not None:
                    nsegs_changed += s._update_node_data()

                    # re-register ions for extracellular species in case new
                    # sections have been added within the ecs region
                    if hasattr(s, "_extracellular_instances"):
                        for ecs in s._extracellular_instances.values():
                            ecs._ion_register()

            if nsegs_changed or newspecies:
                section1d._purge_cptrs()
                for sr in _species_get_all_species():
                    s = sr()
                    if s is not None:
                        s._update_region_indices(True)
                        s._register_cptrs()
                # if species._has_1d and species._1d_submatrix_n():
                _setup_matrices()
                # TODO: separate compiling reactions -- so the indices can be updated without recompiling
                _include_flux(True)
                _setup_units(force=True)
            else:
                # don't call _setup_memb_currents if nsegs changed -- because
                # it is called by change units.
                _setup_memb_currents()


def _matrix_to_rxd_sparse(m):
    """precondition: assumes m a numpy array"""
    nonzero_i, nonzero_j = list(zip(*list(m.keys())))
    nonzero_values = numpy.ascontiguousarray(list(m.values()), dtype=float)

    # number of rows
    n = m.shape[1]

    return (
        n,
        len(nonzero_i),
        numpy.ascontiguousarray(nonzero_i, dtype=ctypes.c_long),
        numpy.ascontiguousarray(nonzero_j, dtype=ctypes.c_long),
        nonzero_values,
    )


def _get_root(sec):
    while sec is not None:
        last_sec = sec
        # technically this is the segment, but need to check non-None
        sec = sec.trueparentseg()
        if sec is not None:
            sec = sec.sec
    return last_sec


def _do_sections_border_each_other(sec1, sec2):
    # two basic ways they could border each other:
    # (1) the distance between a section and the ends
    #     of another section is zero (note: this includes
    #     the case that the sections are the same)
    # (2) a section connects to the interior of another
    for x1, x2 in itertools.product([0, 1], repeat=2):
        if h.distance(sec1(x1), sec2(x2)) == 0:
            # need this check in case the trueparentseg isn't in either
            return True
    if sec1.trueparentseg() in sec2 or sec2.trueparentseg() in sec1:
        return True
    return False


def _do_section_groups_border(groups):
    for g1, g2 in itertools.combinations(groups, 2):
        for sec1 in g1:
            for sec2 in g2:
                if _do_sections_border_each_other(sec1, sec2):
                    return True
    return False


def _check_multigridding_supported_3d():
    # if there are no 3D sections, then all is well
    if not species._has_3d:
        return True

    # if any different dxs are directly connected, then not okay
    groups_by_root_and_dx = {}
    for sr in _species_get_all_species():
        s = sr()
        if s is not None:
            if s._intracellular_instances:
                for r in s._intracellular_instances:
                    dx = r.dx
                    for sec in r._secs3d:
                        root = _get_root(sec)
                        groups_by_root_and_dx.setdefault(root, {})
                        groups_by_root_and_dx[root].setdefault(dx, [])
                        groups_by_root_and_dx[root][dx].append(sec)
    for root, groups in groups_by_root_and_dx.items():
        if _do_section_groups_border(groups.values()):
            return False

    return True


# TODO: make sure this does the right thing when the diffusion constant changes between two neighboring nodes
def _setup_matrices():
    with initializer._init_lock:
        if not _check_multigridding_supported_3d():
            raise RxDException("unsupported multigridding case")

        # update _node_fluxes in C
        _include_flux()

        # TODO: this sometimes seems to get called twice. Figure out why and fix, if possible.

        n = len(_node_get_states())

        volumes = node._get_data()[0]
        zero_volume_indices = (numpy.where(volumes == 0)[0]).astype(ctypes.c_long)
        if species._has_1d:
            # TODO: initialization is slow. track down why
            for sr in _species_get_all_species():
                s = sr()
                if s is not None:
                    s._assign_parents()

            # remove old linearmodeladdition
            n = species._1d_submatrix_n()
            if n:
                # create sparse matrix for C in cy'+gy=b
                c_diagonal = numpy.zeros(n, dtype=ctypes.c_double)
                # most entries are 1 except those corresponding to the 0 and 1 ends

                # create the matrix G
                # if not species._has_3d:
                #    # if we have both, then put the 1D stuff into the matrix that already exists for 3D
                from collections import OrderedDict

                diffusion_matrix = [OrderedDict() for idx in range(n)]
                for sr in _species_get_all_species():
                    s = sr()
                    if s is not None:
                        s._setup_diffusion_matrix(diffusion_matrix)
                        s._setup_c_matrix(c_diagonal)
                        # print '_diffusion_matrix.shape = %r, n = %r, species._has_3d = %r' % (_diffusion_matrix.shape, n, species._has_3d)
                euler_matrix_i, euler_matrix_j, euler_matrix_nonzero = [], [], []
                for i in range(n):
                    mat_i = diffusion_matrix[i]
                    euler_matrix_i.extend(itertools.repeat(i, len(mat_i)))
                    euler_matrix_j.extend(mat_i.keys())
                    euler_matrix_nonzero.extend(mat_i.values())
                euler_matrix_nnonzero = len(euler_matrix_nonzero)
                assert (
                    len(euler_matrix_i)
                    == len(euler_matrix_j)
                    == len(euler_matrix_nonzero)
                )

        # Hybrid logic
        if species._has_1d and species._has_3d:
            hybrid_neighbors = collections.defaultdict(lambda: [])
            hybrid_vols = collections.defaultdict(lambda: [])
            hybrid_diams = {}
            grid_id_dc = {}
            hybrid_index1d_grid_ids = {}
            grid_id_species = {}
            index1d_sec1d = {}
            hybrid_vols1d = {}
            dxs = set()
            for sr in _species_get_all_species():
                s = sr()
                if s is not None:
                    if s._intracellular_instances:
                        # have both 1D and 3D, so find the neighbors
                        # for each of the 3D sections, find the parent sections
                        for r in s._regions:
                            if r in s._intracellular_instances:
                                grid_id = s._intracellular_instances[r]._grid_id
                                grid_id_species.setdefault(
                                    grid_id, s._intracellular_instances[r]
                                )
                                grid_id_dc[grid_id] = s.d
                                dxs.add(r._dx)
                                for sec in r._secs3d:
                                    parent_seg = sec.trueparentseg()
                                    parent_sec = (
                                        None if not parent_seg else parent_seg.sec
                                    )
                                    # are any of these a match with a 1d section?
                                    if s._has_region_section(r, parent_sec):
                                        # this section has a 1d section that is a parent
                                        (
                                            index1d,
                                            indices3d,
                                            vols1d,
                                            vols3d,
                                        ) = _get_node_indices(
                                            s,
                                            r,
                                            sec,
                                            sec.orientation(),
                                            parent_sec,
                                            h.parent_connection(sec=sec),
                                        )
                                        hybrid_neighbors[index1d] += indices3d
                                        hybrid_vols[index1d] += vols3d
                                        hybrid_diams[index1d] = parent_sec(
                                            h.parent_connection(sec=sec)
                                        ).diam
                                        hybrid_index1d_grid_ids[index1d] = grid_id
                                        index1d_sec1d[index1d] = parent_sec
                                        hybrid_vols1d[index1d] = vols1d

                                    for sec1d in r._secs1d:
                                        parent_1d_seg = sec1d.trueparentseg()
                                        parent_1d = (
                                            None
                                            if not parent_1d_seg
                                            else parent_1d_seg.sec
                                        )
                                        if parent_1d == sec:
                                            # it is the parent of a 1d section
                                            (
                                                index1d,
                                                indices3d,
                                                vols1d,
                                                vols3d,
                                            ) = _get_node_indices(
                                                s,
                                                r,
                                                sec,
                                                parent_1d_seg.x,
                                                sec1d,
                                                sec1d.orientation(),
                                            )
                                            hybrid_neighbors[index1d] += indices3d
                                            hybrid_vols[index1d] += vols3d
                                            hybrid_diams[index1d] = sec1d(
                                                h.section_orientation(sec=sec1d)
                                            ).diam
                                            hybrid_index1d_grid_ids[index1d] = grid_id
                                            index1d_sec1d[index1d] = sec1d
                                            hybrid_vols1d[index1d] = vols1d

            if len(dxs) > 1:
                raise RxDException("currently require a unique value for dx")
            dx = dxs.pop()
            rates = []
            volumes3d = []
            volumes1d = []
            grids_dx = []
            hybrid_indices1d = []
            hybrid_indices3d = []
            num_3d_indices_per_1d_seg = []

            num_1d_indices_per_grid = []
            num_3d_indices_per_grid = []

            grid_id_indices1d = collections.defaultdict(lambda: [])
            for index1d in hybrid_neighbors:
                grid_id = hybrid_index1d_grid_ids[index1d]
                grid_id_indices1d[grid_id].append(index1d)
            hybrid_grid_ids = sorted(grid_id_indices1d.keys())
            for grid_id in hybrid_grid_ids:
                sp = grid_id_species[grid_id]
                # TODO: use 3D anisotropic diffusion coefficients
                dc = grid_id_dc[grid_id]
                grids_dx.append(sp._dx**3)
                num_1d_indices_per_grid.append(len(grid_id_indices1d[grid_id]))
                grid_3d_indices_cnt = 0
                for index1d in grid_id_indices1d[grid_id]:
                    neighbors3d = []
                    vols3d = []
                    for neigh, vol in zip(
                        hybrid_neighbors[index1d], hybrid_vols[index1d]
                    ):
                        if neigh not in neighbors3d:
                            neighbors3d.append(neigh)
                            vols3d.append(vol)
                    if not neighbors3d:
                        raise RxDException(
                            "No 3D neighbors detected for 1D segment. Try perturbing dx"
                        )
                    sec1d = index1d_sec1d[index1d]
                    seg_length1d = sec1d.L / sec1d.nseg
                    if neighbors3d:
                        hybrid_indices1d.append(index1d)
                        cnt_neighbors_3d = len(neighbors3d)
                        num_3d_indices_per_1d_seg.append(cnt_neighbors_3d)
                        grid_3d_indices_cnt += cnt_neighbors_3d
                        area = numpy.pi * 0.25 * hybrid_diams[index1d] ** 2
                        areaT = sum([v ** (2.0 / 3.0) for v in vols3d])
                        volumes1d.append(hybrid_vols1d[index1d])
                        for i, vol in zip(neighbors3d, vols3d):
                            sp._region._vol[i] = vol
                            ratio = vol ** (2.0 / 3.0) / areaT
                            rate = ratio * dc * area / (vol * (dx + seg_length1d) / 2)
                            rates.append(rate)
                            volumes3d.append(vol)
                            hybrid_indices3d.append(i)

                num_3d_indices_per_grid.append(grid_3d_indices_cnt)

            num_1d_indices_per_grid = numpy.asarray(
                num_1d_indices_per_grid, dtype=numpy.int64
            )
            num_3d_indices_per_grid = numpy.asarray(
                num_3d_indices_per_grid, dtype=numpy.int64
            )

            hybrid_indices1d = numpy.asarray(hybrid_indices1d, dtype=numpy.int64)
            num_3d_indices_per_1d_seg = numpy.asarray(
                num_3d_indices_per_1d_seg, dtype=numpy.int64
            )
            hybrid_grid_ids.append(-1)
            hybrid_grid_ids = numpy.asarray(hybrid_grid_ids, dtype=numpy.int64)

            hybrid_indices3d = numpy.asarray(hybrid_indices3d, dtype=numpy.int64)
            rates = numpy.asarray(rates, dtype=float)
            volumes1d = numpy.asarray(volumes1d, dtype=float)
            volumes3d = numpy.asarray(volumes3d, dtype=float)
            dxs = numpy.asarray(grids_dx, dtype=float)
            if hybrid_grid_ids.size > 1:
                set_hybrid_data(
                    num_1d_indices_per_grid,
                    num_3d_indices_per_grid,
                    hybrid_indices1d,
                    hybrid_indices3d,
                    num_3d_indices_per_1d_seg,
                    hybrid_grid_ids,
                    rates,
                    volumes1d,
                    volumes3d,
                    dxs,
                )

        # TODO: Replace this this to handle 1d/3d hybrid models
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
        # CRxD
        setup_solver(
            _node_get_states(),
            len(_node_get_states()),
            zero_volume_indices,
            len(zero_volume_indices),
        )
        if species._has_1d and n and euler_matrix_nnonzero > 0:
            section1d._transfer_to_legacy()
            set_euler_matrix(
                n,
                euler_matrix_nnonzero,
                _list_to_clong_array(euler_matrix_i),
                _list_to_clong_array(euler_matrix_j),
                _list_to_cdouble_array(euler_matrix_nonzero),
                c_diagonal,
            )
        else:
            rxd_set_no_diffusion()

        if section1d._all_cindices is not None and section1d._all_cindices:
            rxd_setup_conc_ptrs(
                len(section1d._all_cindices),
                _list_to_cint_array(section1d._all_cindices),
                _list_to_pyobject_array(section1d._all_cptrs),
            )
        else:
            free_conc_ptrs()

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
    # Recalculate the volumes
    xlo, xhi = region._mesh_grid["xlo"], region._mesh_grid["xhi"]
    ylo, yhi = region._mesh_grid["ylo"], region._mesh_grid["yhi"]
    zlo, zhi = region._mesh_grid["zlo"], region._mesh_grid["zhi"]
    from . import geometry3d

    p3d = int((sec3d.n3d() - 1) * x3d)
    p1d = int((sec1d.n3d() - 1) * x1d)
    pt3d = [p3d, p3d + 1] if p3d == 0 else [p3d - 1, p3d]
    pt1d = [p1d, p1d + 1] if p1d == 0 else [p1d - 1, p1d]

    inter, surf, mesh = geometry3d.voxelize2(
        [sec1d, sec3d],
        region._dx,
        mesh_grid=region._mesh_grid,
        relevant_pts=[pt1d, pt3d],
    )

    # TODO: remove need for this assumption
    assert x1d in (0, 1)
    disc_indices = region._indices_from_sec_x(sec3d, x3d)
    # print '%r(%g) connects to the 1d section %r(%g)' % (sec3d, x3d, sec1d, x1d)
    # print 'disc indices: %r' % disc_indices
    indices3d = []
    vols3d = []
    for point in disc_indices:
        if (
            point in _point_indices[region]
            and _point_indices[region][point] not in indices3d
        ):
            indices3d.append(_point_indices[region][point])
            vols3d.append(surf[point][0] if point in surf else region.dx ** 3)
            # print 'found node %d with coordinates (%g, %g, %g)' % (node._index, node.x3d, node.y3d, node.z3d)
    # discard duplicates...
    # TODO: really, need to figure out all the 3d nodes connecting to a given 1d endpoint, then unique that
    # print '3d matrix indices: %r' % indices3d
    # TODO: remove the need for this assertion
    if x1d == sec1d.orientation():
        # TODO: make this whole thing more efficient
        # the parent node is the nonzero index on the first row before the diagonal
        # first_row = min([node._index for node in species.nodes(region)(sec1d)])
        index_1d, vol1d = min(
            [(node._index, node.volume) for node in species.nodes(region)(sec1d)],
            key=lambda x: x[0],
        )
        """for j in range(first_row):
            if _euler_matrix[first_row, j] != 0:
                index_1d = j
                break
        else:
            raise RxDException('should never get here; could not find parent')"""
    elif x1d == 1 - sec1d.orientation():
        # the ending zero-volume node is the one after the last node
        # TODO: make this more efficient
        index_1d, vol1d = max(
            [(node._index, node.volume) for node in species.nodes(region)(sec1d)],
            key=lambda x: x[0],
        )
        index_1d + 1
    else:
        raise RxDException(
            "should never get here; _get_node_indices apparently only partly converted to allow connecting to 1d in middle"
        )
    return index_1d, indices3d, vol1d, vols3d


def _compile_reactions():
    # clear all previous reactions (intracellular & extracellular) and the
    # supporting indexes
    # _windows_remove_dlls()

    regions_inv = dict()  # regions -> reactions that occur there
    species_by_region = dict()
    all_species_involed = set()
    location_count = 0

    ecs_regions_inv = dict()
    ecs_species_by_region = dict()
    ecs_all_species_involed = set()
    ecs_mc_species_involved = set()
    from . import rate, multiCompartmentReaction

    # Find sets of sections that contain the same regions
    from .region import _c_region

    matched_regions = (
        []
    )  # the different combinations of regions that arise in different sections
    rxd_sec_lookup = section1d._SectionLookup()
    for nrnsec in rxd_sec_lookup:
        set_of_regions = set()  # a set of the regions that occur in a given section
        for sec in rxd_sec_lookup[nrnsec]:
            if sec:
                set_of_regions.add(sec._region)
        if set_of_regions not in matched_regions:
            matched_regions.append(set_of_regions)
    region._c_region_lookup = dict()

    # create a c_region instance for each of the unique sets of regions
    c_region_list = []
    for sets in matched_regions:
        c_region_list.append(_c_region(sets))

    for rptr in _all_reactions:
        r = rptr()
        if not r:
            continue

        # Find all the species involved
        if isinstance(r, rate.Rate):
            if not r._species():
                continue
            sptrs = set([r._species])
        else:
            sptrs = set(r._dests + r._sources)

        if hasattr(r, "_involved_species") and r._involved_species:
            sptrs = sptrs.union(set(r._involved_species))
        if hasattr(r, "_involved_species_ecs") and r._involved_species_ecs:
            sptrs = sptrs.union(set(r._involved_species_ecs))
        # Find all the regions involved
        if isinstance(r, multiCompartmentReaction.MultiCompartmentReaction):
            if not hasattr(r._mult, "flatten"):
                r._update_indices()
            react_regions = [
                s()._extracellular()._region
                for s in r._sources + r._dests
                if isinstance(s(), species.SpeciesOnExtracellular)
            ] + [
                s()._region()
                for s in r._sources + r._dests
                if not isinstance(s(), species.SpeciesOnExtracellular)
            ]
            react_regions += [
                sptr()._region()
                for sptr in sptrs
                if isinstance(sptr(), species.SpeciesOnRegion)
            ]
            react_regions += [r._regions[0]]
            react_regions = list(set(react_regions))

        # if regions are specified - use those
        elif hasattr(r, "_active_regions"):
            react_regions = r._active_regions
        # Otherwise use all the regions where the species are
        else:
            react_regions = set()
            nsp = 0
            for sp in sptrs:
                s = sp()
                nsp += 1
                if isinstance(s, species.SpeciesOnRegion):
                    react_regions.add(s._region())
                elif isinstance(s, species.SpeciesOnExtracellular):
                    react_regions.add(s._extracellular()._region)
                elif isinstance(s, species._ExtracellularSpecies):
                    react_regions.add(s._region)
                elif None not in s._regions:
                    [
                        react_regions.add(reg)
                        for reg in s._regions + s._extracellular_regions
                    ]
            react_regions = list(react_regions)
            # Only regions where ALL the species are present -- unless it is a membrane
            # from collections import Counter
            # from . import geometry as geo
            # react_regions = [reg for reg, count in Counter(react_regions).iteritems() if count == nsp or isinstance(reg.geometry,geo.ScalableBorder)]
        # Any intracellular regions
        if not all([isinstance(x, region.Extracellular) for x in react_regions]):
            species_involved = []
            for sp in sptrs:
                s = sp()
                if not isinstance(s, species.SpeciesOnExtracellular) and not isinstance(
                    s, species._ExtracellularSpecies
                ):
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
                    species_by_region[reg] = species_by_region[reg].union(
                        species_involved
                    )
                else:
                    species_by_region[reg] = set(species_involved)
                    for sec in reg._secs:
                        location_count += sec.nseg
        # Any extracellular regions
        if any([isinstance(x, region.Extracellular) for x in react_regions]):
            # MultiCompartment - so can have both extracellular and intracellular regions
            if isinstance(r, multiCompartmentReaction.MultiCompartmentReaction):
                for sp in sptrs:
                    s = sp()
                    if isinstance(s, species.SpeciesOnExtracellular):
                        ecs_mc_species_involved.add(s)
                    if isinstance(s, species.Species) and s._extracellular_instances:
                        for ecs in s._extracellular_instances.keys():
                            ecs_mc_species_involved.add(s[ecs])
                for reg in react_regions:
                    if reg in list(ecs_species_by_region.keys()):
                        ecs_species_by_region[reg] = ecs_species_by_region[reg].union(
                            ecs_mc_species_involved
                        )
                    else:
                        ecs_species_by_region[reg] = set(ecs_mc_species_involved)
            # Otherwise - reaction can only have extracellular regions
            else:
                ecs_species_involved = []
                for sp in sptrs:
                    s = sp()
                    ecs_all_species_involed.add(s)
                    ecs_species_involved.append(s)
                for reg in react_regions:
                    if not isinstance(reg, region.Extracellular):
                        continue
                    if reg in ecs_regions_inv:
                        ecs_regions_inv[reg].append(rptr)
                    else:
                        ecs_regions_inv[reg] = [rptr]
                    if reg in ecs_species_by_region:
                        ecs_species_by_region[reg] = ecs_species_by_region[reg].union(
                            ecs_species_involved
                        )
                    else:
                        ecs_species_by_region[reg] = set(ecs_species_involved)
    # Create lists of indexes for intracellular reactions and rates
    # a table for location,species -> state index
    regions_inv_1d = [reg for reg in regions_inv if any(reg._secs1d)]
    regions_inv_1d.sort(key=lambda r: r._id)
    all_regions_inv_3d = [reg for reg in regions_inv if any(reg._secs3d)]
    # remove extra regions from multicompartment reactions. We only want the membrane
    regions_inv_3d = set()
    for reg in all_regions_inv_3d:
        for rptr in regions_inv[reg]:
            r = rptr()
            if isinstance(r, multiCompartmentReaction.MultiCompartmentReaction):
                regions_inv_3d.add(r._regions[0])
            else:
                regions_inv_3d.add(reg)
    regions_inv_3d = list(regions_inv_3d)

    for reg in regions_inv_1d:
        rptr = weakref.ref(reg)
        if rptr in region._c_region_lookup:
            for c_region in region._c_region_lookup[rptr]:
                for react in regions_inv[reg]:
                    c_region.add_reaction(react, rptr)
                    c_region.add_species(species_by_region[reg])
                    if reg in ecs_species_by_region:
                        c_region.add_ecs_species(ecs_species_by_region[reg])

    # now setup the reactions
    # if there are no reactions
    if location_count == 0 and len(ecs_regions_inv) == 0:
        return None

    def localize_index(creg, rate):
        rate_str = re.sub(
            r"species\[(\d+)\]\[(\d+)\]",
            lambda m: "species[%i][%i]"
            % (
                creg._species_ids.get(int(m.groups()[0])),
                creg._region_ids.get(int(m.groups()[1])),
            ),
            rate,
        )
        rate_str = re.sub(
            r"params\[(\d+)\]\[(\d+)\]",
            lambda m: "params[%i][%i]"
            % (
                creg._params_ids.get(int(m.groups()[0])),
                creg._region_ids.get(int(m.groups()[1])),
            ),
            rate_str,
        )
        rate_str = re.sub(
            r"species_3d\[(\d+)\]",
            lambda m: "species_3d[%i]" % creg._ecs_species_ids.get(int(m.groups()[0])),
            rate_str,
        )
        rate_str = re.sub(
            r"params_3d\[(\d+)\]",
            lambda m: "params_3d[%i]" % creg._ecs_params_ids.get(int(m.groups()[0])),
            rate_str,
        )
        return rate_str

    # Setup intracellular and multicompartment reactions
    if location_count > 0:
        from . import rate, multiCompartmentReaction, Parameter

        for creg in c_region_list:
            if not creg._react_regions:
                continue
            creg._initalize()
            mc_mult_count = 0
            mc_mult_list = []
            species_ids_used = numpy.zeros((creg.num_species, creg.num_regions), bool)
            flux_ids_used = numpy.zeros((creg.num_species, creg.num_regions), bool)
            ecs_species_ids_used = numpy.zeros((creg.num_ecs_species), bool)
            fxn_string = _c_headers
            fxn_string += "void reaction(double** species, double** params, double** rhs, double* mult, double* species_3d, double* params_3d, double* rhs_3d, double** flux, double v)\n{"
            # declare the "rate" variable if any reactions (non-rates)
            for rprt in creg._react_regions:
                if not isinstance(rprt(), rate.Rate):
                    fxn_string += "\n\tdouble rate;"
                    break
            for rptr in _all_reactions:
                if rptr not in creg._react_regions:
                    continue
                r = rptr()
                if isinstance(r, rate.Rate):
                    s = r._species()
                    if s._id in creg._params_ids:
                        warn(
                            "Parameters values are fixed, %r will not change the value of %r"
                            % (r, s)
                        )
                        continue
                    species_id = creg._species_ids[s._id]
                    for reg in creg._react_regions[rptr]:
                        if reg() in r._rate:
                            try:
                                region_id = creg._region_ids[reg()._id]
                                rate_str = localize_index(creg, r._rate[reg()][0])
                            except KeyError:
                                warn(
                                    "Species not on the region specified, %r will be ignored.\n"
                                    % r
                                )
                                continue
                            operator = (
                                "+=" if species_ids_used[species_id][region_id] else "="
                            )
                            fxn_string += "\n\trhs[%d][%d] %s %s;" % (
                                species_id,
                                region_id,
                                operator,
                                rate_str,
                            )
                            species_ids_used[species_id][region_id] = True
                elif isinstance(r, multiCompartmentReaction.MultiCompartmentReaction):
                    # Lookup the region_id for the reaction
                    try:
                        for reg in r._rate:
                            rate_str = localize_index(creg, r._rate[reg][0])
                            fxn_string += "\n\trate = %s;" % rate_str
                            break
                    except KeyError:
                        warn(
                            "Species not on the region specified, %r will be ignored.\n"
                            % r
                        )
                        continue

                    for i, sptr in enumerate(r._sources + r._dests):
                        s = sptr()
                        if isinstance(s, species.SpeciesOnExtracellular):
                            if not isinstance(s, species.ParameterOnExtracellular):
                                species_id = creg._ecs_species_ids[
                                    s._extracellular()._grid_id
                                ]
                                operator = (
                                    "+=" if ecs_species_ids_used[species_id] else "="
                                )
                                fxn_string += "\n\trhs_3d[%d] %s mult[%d] * rate;" % (
                                    species_id,
                                    operator,
                                    mc_mult_count,
                                )
                                ecs_species_ids_used[species_id] = True
                        elif not isinstance(s, species.Parameter) and not isinstance(
                            s, species.ParameterOnRegion
                        ):
                            species_id = creg._species_ids[s._id]
                            region_id = creg._region_ids[s._region()._id]
                            operator = (
                                "+=" if species_ids_used[species_id][region_id] else "="
                            )
                            fxn_string += "\n\trhs[%d][%d] %s mult[%d] * rate;" % (
                                species_id,
                                region_id,
                                operator,
                                mc_mult_count,
                            )
                            species_ids_used[species_id][region_id] = True
                            if r._membrane_flux:
                                operator = (
                                    "+="
                                    if flux_ids_used[species_id][region_id]
                                    else "="
                                )
                                fxn_string += (
                                    "\n\tif(flux) flux[%d][%d] %s %1.1f * rate;"
                                    % (
                                        species_id,
                                        region_id,
                                        operator,
                                        r._cur_charges[i],
                                    )
                                )
                                flux_ids_used[species_id][region_id] = True
                            # TODO: Fix problem if the whole region isn't part of the same aggregate c_region
                        mc_mult_count += 1
                    mc_mult_list.extend(r._mult.flatten())
                else:
                    for reg in creg._react_regions[rptr]:
                        try:
                            region_id = creg._region_ids[reg()._id]
                            rate_str = localize_index(creg, r._rate[reg()][0])
                        except KeyError:
                            warn(
                                "Species not on the region specified, %r will be ignored.\n"
                                % r
                            )
                            continue
                        fxn_string += "\n\trate = %s;" % rate_str
                        summed_mults = collections.defaultdict(lambda: 0)
                        for mult, sp in zip(r._mult, r._sources + r._dests):
                            summed_mults[creg._species_ids.get(sp()._id)] += mult
                        for idx in sorted([k for k in summed_mults if k is not None]):
                            operator = "+=" if species_ids_used[idx][region_id] else "="
                            species_ids_used[idx][region_id] = True
                            fxn_string += "\n\trhs[%d][%d] %s (%g) * rate;" % (
                                idx,
                                region_id,
                                operator,
                                summed_mults[idx],
                            )
            fxn_string += "\n}\n}\n"
            register_rate(
                creg.num_species,
                creg.num_params,
                creg.num_regions,
                creg.num_segments,
                creg.get_state_index(),
                creg.num_ecs_species,
                creg.num_ecs_params,
                creg.get_ecs_species_ids(),
                creg.get_ecs_index(),
                mc_mult_count,
                numpy.array(mc_mult_list, dtype=ctypes.c_double),
                _list_to_pyobject_array(creg._vptrs),
                _cxx_compile(fxn_string),
            )

    # Setup intracellular 3D reactions
    molecules_per_mM_um3 = constants.molecules_per_mM_um3()
    if regions_inv_3d:
        for reg in regions_inv_3d:
            ics_grid_ids = []
            all_ics_gids = set()
            ics_param_gids = set()
            fxn_string = _c_headers
            fxn_string += "void reaction(double* species_3d, double* params_3d, double*rhs, double* mc3d_mults)\n{"
            for rptr in [r for rlist in list(regions_inv.values()) for r in rlist]:
                if not isinstance(rptr(), rate.Rate):
                    fxn_string += "\n\tdouble rate;\n"
                    break
            # if any rates on this region have SpeciesOnRegion, add their grid_ids
            # do this in loop above if it is correct
            for rptr in [r for rlist in list(regions_inv.values()) for r in rlist]:
                r = rptr()
                if isinstance(r, rate.Rate):
                    if reg in r._regions:
                        for spec_involved in r._involved_species:
                            # probably should do parameters/states here as well
                            if isinstance(spec_involved(), species.SpeciesOnRegion):
                                all_ics_gids.add(
                                    spec_involved()
                                    ._species()
                                    ._intracellular_instances[spec_involved()._region()]
                                    ._grid_id
                                )
                elif isinstance(r, multiCompartmentReaction.MultiCompartmentReaction):
                    if reg in r._rate:
                        for spec_involved in (
                            r._involved_species + r._sources + r._dests
                        ):
                            all_ics_gids.add(
                                spec_involved()
                                ._species()
                                ._intracellular_instances[spec_involved()._region()]
                                ._grid_id
                            )

            for s in species_by_region[reg]:
                spe = s._species() if isinstance(s, species.SpeciesOnRegion) else s
                if (
                    hasattr(spe, "_intracellular_instances")
                    and spe._intracellular_instances
                    and reg in spe._intracellular_instances
                ):
                    if isinstance(s, species.Parameter) or isinstance(
                        s, species.ParameterOnRegion
                    ):
                        sp = spe._intracellular_instances[reg]
                        ics_param_gids.add(sp._grid_id)
                    else:
                        ###TODO is this correct? are there any other cases I should worry about? Do I always make a species the intracellular instance for the region we are looping through?
                        sp = spe._intracellular_instances[reg]
                        all_ics_gids.add(sp._grid_id)
            all_ics_gids = list(all_ics_gids)
            ics_param_gids = list(ics_param_gids)
            if any(
                [
                    isinstance(
                        rptr(), multiCompartmentReaction.MultiCompartmentReaction
                    )
                    for rptr in regions_inv[reg]
                ]
            ):
                # the elements in each list contain the indices into the states vector for the intracellular instance that need to be updated
                mc3d_region_size = len(reg._xs)
                mc3d_indices_start = [
                    species._defined_species_by_gid[index]._mc3d_indices_start(reg)
                    for index in all_ics_gids + ics_param_gids
                ]
            else:
                mc3d_region_size = 0
                mc3d_indices_start = [0] * (len(all_ics_gids) + len(ics_param_gids))
            mults = [[] for i in range(len(all_ics_gids + ics_param_gids))]
            for rptr in regions_inv[reg]:
                r = rptr()
                if reg not in r._rate:
                    continue
                rate_str = re.sub(
                    r"species_3d\[(\d+)\]",
                    lambda m: "species_3d[%i]"
                    % [
                        pid
                        for pid, gid in enumerate(all_ics_gids)
                        if gid == int(m.groups()[0])
                    ][0],
                    r._rate[reg][-1],
                )
                rate_str = re.sub(
                    r"params_3d\[(\d+)\]",
                    lambda m: "params_3d[%i]"
                    % [
                        pid
                        for pid, gid in enumerate(ics_param_gids)
                        if gid == int(m.groups()[0])
                    ][0],
                    rate_str,
                )
                if isinstance(r, rate.Rate):
                    s = r._species()
                    # Get underlying rxd._IntracellularSpecies for the grid_id
                    if isinstance(s, species.Parameter) or isinstance(
                        s, species.ParameterOnRegion
                    ):
                        continue
                    elif isinstance(s, species.Species):
                        s = s._intracellular_instances[reg]
                    elif isinstance(s, species.SpeciesOnRegion):
                        s = s._species()._intracellular_instances[s._region()]
                    if s._grid_id in ics_grid_ids:
                        operator = "+="
                    else:
                        operator = "="
                        ics_grid_ids.append(s._grid_id)
                    pid = [
                        pid for pid, gid in enumerate(all_ics_gids) if gid == s._grid_id
                    ][0]
                    fxn_string += "\n\trhs[%d] %s %s;" % (pid, operator, rate_str)
                elif isinstance(r, multiCompartmentReaction.MultiCompartmentReaction):
                    if reg in r._regions:
                        from . import geometry

                        fxn_string += "\n\trate = " + rate_str + ";"
                        for sptr in r._sources:
                            s = sptr()
                            if not isinstance(s, species.Parameter) and not isinstance(
                                s, species.ParameterOnRegion
                            ):
                                s3d = s.instance3d
                                if s3d._grid_id in ics_grid_ids:
                                    operator = "+="
                                else:
                                    operator = "="
                                    ics_grid_ids.append(s3d._grid_id)
                                    # Find mult for this grid
                                    sas = reg._vol
                                    s3d_reg = s3d._region
                                    for segidx, seg in enumerate(s3d_reg._segs3d()):
                                        # Change this to be by volume
                                        # membrane area / compartment volume / molecules_per_mM_um3
                                        mults[s3d._grid_id] += [
                                            sas[index]
                                            / (s3d_reg._vol[index])
                                            / molecules_per_mM_um3
                                            for index in s3d_reg._nodes_by_seg[segidx]
                                        ]

                                pid = [
                                    pid
                                    for pid, gid in enumerate(all_ics_gids)
                                    if gid == s3d._grid_id
                                ][0]
                                fxn_string += (
                                    "\n\trhs[%d] %s -mc3d_mults[%d] * rate;"
                                    % (pid, operator, pid)
                                )
                        for sptr in r._dests:
                            s = sptr()
                            if not isinstance(s, species.Parameter) and not isinstance(
                                s, species.ParameterOnRegion
                            ):
                                s3d = s.instance3d
                                if s3d._grid_id in ics_grid_ids:
                                    operator = "+="
                                else:
                                    operator = "="
                                    ics_grid_ids.append(s3d._grid_id)
                                    # Find mult for this grid
                                    sas = reg._vol
                                    s3d_reg = s3d._region
                                    for segidx, seg in enumerate(s3d_reg._segs3d()):
                                        # Change this to be by volume
                                        mults[s3d._grid_id] += [
                                            sas[index]
                                            / (s3d_reg._vol[index])
                                            / molecules_per_mM_um3
                                            for index in s3d_reg._nodes_by_seg[segidx]
                                        ]
                                pid = [
                                    pid
                                    for pid, gid in enumerate(all_ics_gids)
                                    if gid == s3d._grid_id
                                ][0]
                                fxn_string += (
                                    "\n\trhs[%d] %s mc3d_mults[%d] * rate;"
                                    % (pid, operator, pid)
                                )

                else:
                    idx = 0
                    fxn_string += f"\n\trate = {rate_str};"
                    for sp in r._sources + r._dests:
                        s = sp()
                        # Get underlying rxd._IntracellularSpecies for the grid_id
                        if isinstance(s, species.Parameter) or isinstance(
                            s, species.ParameterOnRegion
                        ):
                            idx += 1
                            continue
                        if isinstance(s, species.Species):
                            s = s._intracellular_instances[reg]
                        elif isinstance(s, species.SpeciesOnRegion):
                            if s._region() in s._species()._intracellular_instances:
                                s = s._species()._intracellular_instances[s._region()]
                            else:
                                continue
                        if s._grid_id in ics_grid_ids:
                            operator = "+="
                        else:
                            operator = "="
                            ics_grid_ids.append(s._grid_id)
                        pid = [
                            pid
                            for pid, gid in enumerate(all_ics_gids)
                            if gid == s._grid_id
                        ][0]
                        fxn_string += "\n\trhs[%d] %s (%s)*rate;" % (
                            pid,
                            operator,
                            r._mult[idx],
                        )
                        idx += 1
            fxn_string += "\n}\n}\n"
            for i, ele in enumerate(mults):
                if ele == []:
                    mults[i] = numpy.ones(len(reg._xs))
            mults = list(itertools.chain.from_iterable(mults))
            ics_register_reaction(
                0,
                len(all_ics_gids),
                len(ics_param_gids),
                _list_to_cint_array(all_ics_gids + ics_param_gids),
                numpy.asarray(mc3d_indices_start, dtype=numpy.uint64),
                mc3d_region_size,
                numpy.asarray(mults),
                _cxx_compile(fxn_string),
            )
    # Setup extracellular reactions
    if len(ecs_regions_inv) > 0:
        for reg in ecs_regions_inv:
            grid_ids = []
            all_gids = set()
            param_gids = set()
            fxn_string = _c_headers
            # TODO: find the nrn include path in python
            # It is necessary for a couple of function in python that are not in math.h
            fxn_string += (
                "void reaction(double* species_3d, double* params_3d, double* rhs)\n{"
            )
            # declare the "rate" variable if any reactions (non-rates)
            for rptr in [r for rlist in list(ecs_regions_inv.values()) for r in rlist]:
                if not isinstance(rptr(), rate.Rate):
                    fxn_string += "\n\tdouble rate;"
                    break
            # get a list of all grid_ids involved
            for s in ecs_species_by_region[reg]:
                if isinstance(s, species.Parameter) or isinstance(
                    s, species.ParameterOnExtracellular
                ):
                    sp = s[reg] if isinstance(s, species.Species) else s
                    param_gids.add(
                        sp._extracellular()._grid_id
                        if isinstance(sp, species.SpeciesOnExtracellular)
                        else sp._grid_id
                    )
                else:
                    sp = s[reg] if isinstance(s, species.Species) else s
                    all_gids.add(
                        sp._extracellular()._grid_id
                        if isinstance(sp, species.SpeciesOnExtracellular)
                        else sp._grid_id
                    )
            all_gids = list(all_gids)
            param_gids = list(param_gids)
            for rptr in ecs_regions_inv[reg]:
                r = rptr()
                rate_str = re.sub(
                    r"species_3d\[(\d+)\]",
                    lambda m: "species_3d[%i]"
                    % [
                        pid
                        for pid, gid in enumerate(all_gids)
                        if gid == int(m.groups()[0])
                    ][0],
                    r._rate_ecs[reg][-1],
                )
                rate_str = re.sub(
                    r"params_3d\[(\d+)\]",
                    lambda m: "params_3d[%i]"
                    % [
                        pid
                        for pid, gid in enumerate(param_gids)
                        if gid == int(m.groups()[0])
                    ][0],
                    rate_str,
                )
                if isinstance(r, rate.Rate):
                    s = r._species()
                    # Get underlying rxd._ExtracellularSpecies for the grid_id
                    if isinstance(s, species.Parameter) or isinstance(
                        s, species.ParameterOnExtracellular
                    ):
                        continue
                    elif isinstance(s, species.Species):
                        s = s[reg]._extracellular()
                    elif isinstance(s, species.SpeciesOnExtracellular):
                        s = s._extracellular()
                    if s._grid_id in grid_ids:
                        operator = "+="
                    else:
                        operator = "="
                        grid_ids.append(s._grid_id)
                    pid = [
                        pid for pid, gid in enumerate(all_gids) if gid == s._grid_id
                    ][0]
                    fxn_string += f"\n\trhs[{pid}] {operator} {rate_str};"
                else:
                    idx = 0
                    fxn_string += f"\n\trate = {rate_str};"
                    for sp in r._sources + r._dests:
                        s = sp()
                        # Get underlying rxd._ExtracellularSpecies for the grid_id
                        if isinstance(s, species.Parameter) or isinstance(
                            s, species.ParameterOnExtracellular
                        ):
                            idx += 1
                            continue
                        if isinstance(s, species.Species):
                            s = s[reg]._extracellular()
                        elif isinstance(s, species.SpeciesOnExtracellular):
                            s = s._extracellular()
                        if s._grid_id in grid_ids:
                            operator = "+="
                        else:
                            operator = "="
                            grid_ids.append(s._grid_id)
                        pid = [
                            pid for pid, gid in enumerate(all_gids) if gid == s._grid_id
                        ][0]
                        fxn_string += "\n\trhs[%d] %s (%s)*rate;" % (
                            pid,
                            operator,
                            r._mult[idx],
                        )
                        idx += 1
            fxn_string += "\n}\n}\n"
            ecs_register_reaction(
                0,
                len(all_gids),
                len(param_gids),
                _list_to_cint_array(all_gids + param_gids),
                _cxx_compile(fxn_string),
            )


def _init():
    if not species._all_species:
        return None
    initializer._do_init()
    # TODO: check about the 0<x<1 problem alluded to in the documentation
    h.define_shape()

    # if the shape has changed update the nodes
    _update_node_data()

    if species._has_1d:
        section1d._purge_cptrs()

    for sr in _species_get_all_species():
        s = sr()
        if s is not None:
            # TODO: are there issues with hybrid or 3D here? (I don't think so, but here's a bookmark just in case)
            s._register_cptrs()
            s._finitialize()
    _setup_matrices()
    # if species._has_1d and species._1d_submatrix_n():
    # volumes = node._get_data()[0]
    # zero_volume_indices = (numpy.where(volumes == 0)[0]).astype(ctypes.c_long)
    # setup_solver(_node_get_states(), len(_node_get_states()), zero_volume_indices, len(zero_volume_indices), h._ref_t, h._ref_dt)
    clear_rates()
    _setup_memb_currents()
    _compile_reactions()


def _include_flux(force=False):
    from .node import _node_fluxes
    from . import node

    if force or node._has_node_fluxes:
        index1D = []
        source1D = []
        scale1D = []
        grids = dict()
        for idx, t, src, sc, rptr in zip(
            _node_fluxes["index"],
            _node_fluxes["type"],
            _node_fluxes["source"],
            _node_fluxes["scale"],
            _node_fluxes["region"],
        ):
            if t == -1:
                index1D.append(idx)
                source1D.append(src)
                scale1D.append(sc * node._volumes[idx])
            else:
                gid = t
                if gid not in grids:
                    grids[gid] = {"index": [], "source": [], "scale": []}
                grids[gid]["index"].append(idx)
                grids[gid]["source"].append(src)
                grids[gid]["scale"].append(sc * rptr().volume(idx))
        counts3D = []
        grids3D = sorted(grids.keys())
        index3D = []
        source3D = []
        scale3D = []
        for gid in grids3D:
            counts3D.append(len(grids[gid]["index"]))
            index3D.extend(grids[gid]["index"])
            source3D.extend(grids[gid]["source"])
            scale3D.extend(grids[gid]["scale"])
        rxd_include_node_flux1D(
            len(index1D),
            _list_to_clong_array(index1D),
            _list_to_cdouble_array(scale1D),
            _list_to_pyobject_array(source1D),
        )

        rxd_include_node_flux3D(
            len(grids3D),
            _list_to_cint_array(counts3D),
            _list_to_cint_array(grids3D),
            _list_to_clong_array(index3D),
            _list_to_cdouble_array(scale3D),
            _list_to_pyobject_array(source3D),
        )
        node._has_node_fluxes = False


def _init_concentration():
    if not species._all_species:
        return None
    for sr in _species_get_all_species():
        s = sr()
        if s is not None:
            # TODO: are there issues with hybrid or 3D here? (I don't think so, but here's a bookmark just in case)
            s._finitialize()


_has_nbs_registered = False
_nbs = None
do_setup_matrices_fptr = None
do_setup_units_fptr = None


def _do_nbs_register():
    global _has_nbs_registered, _nbs, _fih, _fih2, _fih3, do_setup_matrices_fptr, do_setup_units_fptr

    if not _has_nbs_registered:
        # from neuron import nonvint_block_supervisor as _nbs

        _has_nbs_registered = True
        # _nbs.register(_callbacks)  not used

        #
        # register the initialization handler and the ion register handler
        #
        _fih = h.FInitializeHandler(_init_concentration)
        _fih3 = h.FInitializeHandler(3, _init)

        set_setup_matrices = nrn_dll_sym("set_setup_matrices")
        set_setup_matrices.argtypes = [fptr_prototype]
        do_setup_matrices_fptr = fptr_prototype(_setup_matrices)
        set_setup_matrices(do_setup_matrices_fptr)

        set_setup_units = nrn_dll_sym("set_setup_units")
        set_setup_units.argtypes = [fptr_prototype]
        do_setup_units_fptr = fptr_prototype(_setup_units)
        set_setup_units(do_setup_units_fptr)

        _fih2 = h.FInitializeHandler(3, initializer._do_ion_register)

        #
        # register scatter/gather mechanisms
        #
        _cvode_object.extra_scatter_gather(0, _after_advance)

        # register save state mechanism
        import neuron
        from neuron import rxd

        neuron.register_savestate(
            "rxd-9e015bfa-93ba-485c-9dd2-09857ae58e4d",
            rxd.save_state,
            rxd.restore_state,
        )


# register the Python callbacks
do_setup_fptr = fptr_prototype(_setup)
do_initialize_fptr = fptr_prototype(_init)
set_setup(do_setup_fptr)
set_initialize(do_initialize_fptr)


def _windows_remove_dlls():
    global _windows_dll_files, _windows_dll
    if _windows_dll != []:
        if hasattr(ctypes, "WinDLL"):
            kernel32 = ctypes.WinDLL("kernel32")
            kernel32.FreeLibrary.argtypes = [ctypes.c_void_p]
        else:
            kernel32 = ctypes.windll.kernel32
    for dll_ptr, filepath in zip(_windows_dll, _windows_dll_files):
        dll = dll_ptr()
        if dll:
            handle = dll._handle
            del dll
            kernel32.FreeLibrary(handle)
        try:
            os.remove(filepath)
        except:
            pass
    _windows_dll_files = []
    _windows_dll = []


def nthread(n=None):
    if n:
        _set_num_threads(n)
    return _get_num_threads()

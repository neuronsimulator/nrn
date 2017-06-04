import neuron
from neuron import h, nrn
from . import species, node, section1d, region, morphology
from .nodelist import NodeList
import weakref
import numpy
import scipy.sparse
import scipy.sparse.linalg
import ctypes
import atexit
from . import options
from .rxdException import RxDException
from . import initializer 
import collections

# aliases to avoid repeatedly doing multiple hash-table lookups
_numpy_array = numpy.array
_numpy_zeros = numpy.zeros
_scipy_sparse_linalg_bicgstab = scipy.sparse.linalg.bicgstab
_scipy_sparse_eye = scipy.sparse.eye
_scipy_sparse_linalg_spsolve = scipy.sparse.linalg.spsolve
_scipy_sparse_dok_matrix = scipy.sparse.dok_matrix
_scipy_sparse_linalg_factorized = scipy.sparse.linalg.factorized
_scipy_sparse_coo_matrix = scipy.sparse.coo_matrix
_species_get_all_species = species._get_all_species
_node_get_states = node._get_states
_section1d_transfer_to_legacy = section1d._transfer_to_legacy
_ctypes_c_int = ctypes.c_int
_weakref_ref = weakref.ref

_external_solver = None
_external_solver_initialized = False

def byeworld():
    # needed to prevent a seg-fault error at shudown in at least some
    # combinations of NEURON and Python, which I think is due to objects
    # getting deleted out-of-order
    global _react_matrix_solver
    try:
        del _react_matrix_solver
    except NameError:
        # if it already didn't exist, that's fine
        pass
    
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

_linmodadd = None
_linmodadd_c = None
_diffusion_matrix = None
_curr_scales = None
_curr_ptrs = None
_curr_indices = None

_all_reactions = []

_zero_volume_indices = []
_nonzero_volume_indices = []

_double_ptr = ctypes.POINTER(ctypes.c_double)
_int_ptr = ctypes.POINTER(_ctypes_c_int)


nrn_tree_solve = neuron.nrn_dll_sym('nrn_tree_solve')
nrn_tree_solve.restype = None

_dptr = _double_ptr

_dimensions = collections.defaultdict(lambda: 1)
_default_dx = 0.25
_default_method = 'deterministic'

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
    global last_structure_change_cnt, _diffusion_matrix, _external_solver_initialized
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
    global last_structure_change_cnt
    global _rxd_offset
    initializer._do_init()
    _rxd_offset = offset
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
    states = _node_get_states()
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
    for sr in list(_species_get_all_species().values()):
        s = sr()
        if s is not None: s._transfer_to_legacy()

    
    if ydot is not None:
        # diffusion_matrix = - jacobian    
        ydot[lo : hi] = (_rxd_reaction(states) - _diffusion_matrix * states)[_nonzero_volume_indices]
        
    states[_zero_volume_indices] = 0

def _ode_solve(dt, t, b, y):
    initializer.assert_initialized()
    if _diffusion_matrix is None: _setup_matrices()
    lo = _rxd_offset
    hi = lo + len(_nonzero_volume_indices)
    n = len(_node_get_states())
    # TODO: this will need changed when can have both 1D and 3D
    if species._has_3d:
        if species._has_1d:
            raise Exception('development issue: cvode currently does not support hybrid simulations (fix by shifting for zero volume indices)')
        # NOTE: only working on the rxd part
        rxd_b = b[lo : hi]
        # TODO: make sure can handle both 1D and 3D
        m = eye_minus_dt_J(n, dt)
        # removed diagonal preconditioner since tests showed no improvement in convergence
        result, info = _scipy_sparse_linalg_bicgstab(m, dt * rxd_b)
        assert(info == 0)
        b[lo : hi] = _react_matrix_solver(result)
    else:
        # 1D only; use Hines solver
        full_b = numpy.zeros(n)
        full_b[_nonzero_volume_indices] = b[lo : hi]
        b[lo : hi] = _react_matrix_solver(_diffusion_matrix_solve(dt, full_b))[_nonzero_volume_indices]
        # the following version computes the reaction matrix each time
        #full_y = numpy.zeros(n)
        #full_y[_nonzero_volume_indices] = y[lo : hi]
        #b[lo : hi] = _reaction_matrix_solve(dt, full_y, _diffusion_matrix_solve(dt, full_b))[_nonzero_volume_indices]
        # this line doesn't include the reaction contributions to the Jacobian
        #b[lo : hi] = _diffusion_matrix_solve(dt, full_b)[_nonzero_volume_indices]

_rxd_induced_currents = None

def _currents(rhs):
    initializer._do_init()
    # setup membrane fluxes from our stuff
    # TODO: cache the memb_cur_ptrs, memb_cur_charges, memb_net_charges, memb_cur_mapped
    #       because won't change very often
    global _rxd_induced_currents

    # need this; think it's because of initialization of mod files
    if _curr_indices is None: return

    # TODO: change so that this is only called when there are in fact currents
    _rxd_induced_currents = _numpy_zeros(len(_curr_indices))
    rxd_memb_flux = []
    memb_cur_ptrs = []
    memb_cur_charges = []
    memb_net_charges = []
    memb_cur_mapped = []
    for rptr in _all_reactions:
        r = rptr()
        if r and r._membrane_flux:
            # NOTE: memb_flux contains any scaling we need
            new_fluxes = r._get_memb_flux(_node_get_states())
            rxd_memb_flux += list(new_fluxes)
            memb_cur_ptrs += r._cur_ptrs
            memb_cur_mapped += r._cur_mapped
            memb_cur_charges += [r._cur_charges] * len(new_fluxes)
            memb_net_charges += [r._net_charges] * len(new_fluxes)

    # TODO: is this in any way dimension dependent?

    if rxd_memb_flux:
        # TODO: remove the asserts when this is verified to work
        assert(len(rxd_memb_flux) == len(_cur_node_indices))
        assert(len(rxd_memb_flux) == len(memb_cur_ptrs))
        assert(len(rxd_memb_flux) == len(memb_cur_charges))
        assert(len(rxd_memb_flux) == len(memb_net_charges))
        for flux, cur_ptrs, cur_charges, net_charge, i, cur_maps in zip(rxd_memb_flux, memb_cur_ptrs, memb_cur_charges, memb_net_charges, _cur_node_indices, memb_cur_mapped):
            rhs[i] -= net_charge * flux
            #print net_charge * flux
            #import sys
            #sys.exit()
            # TODO: remove this assert when more thoroughly tested
            assert(len(cur_ptrs) == len(cur_maps))
            for ptr, charge, cur_map_i in zip(cur_ptrs, cur_charges, cur_maps):
                # this has the opposite sign of the above because positive
                # currents lower the membrane potential
                cur = charge * flux
                ptr[0] += cur
                for sign, c in zip([-1, 1], cur_maps):
                    if c is not None:
                        _rxd_induced_currents[c] += sign * cur

_last_m = None
_last_preconditioner = None
_fixed_step_count = 0
from scipy.sparse.linalg import spilu as _spilu
from scipy.sparse.linalg import LinearOperator as _LinearOperator
from scipy.sparse import csc_matrix

def eye_minus_dt_J(n, dt):
    """correctly computes I - dt J as needed for the lhs of an advance.
    
    The difficulty here is that the _euler_matrix also contains conservation
    equations. These are preserved unchanged (i.e. no +1).
    
    This reads two globals: _euler_matrix and _zero_volume_indices.
    
    n is the length of the state vector (including the conservation nodes).
    """
    m = _scipy_sparse_eye(n, n) - dt * _euler_matrix
    # correct to account for algebraic conservation nodes which don't get the +1
    for i in _zero_volume_indices:
        m[i, i] -= 1
    return m

    

def _fixed_step_solve(raw_dt):
    initializer._do_init()
    global pinverse, _fixed_step_count
    global _last_m, _last_dt, _last_preconditioner

    if species._species_count == 0:
        return

    # allow for skipping certain fixed steps
    # warning: this risks numerical errors!
    fixed_step_factor = options.fixed_step_factor
    _fixed_step_count += 1
    if _fixed_step_count % fixed_step_factor: return
    dt = fixed_step_factor * raw_dt
    
    # TODO: this probably shouldn't be here
    if _diffusion_matrix is None and _euler_matrix is None: _setup_matrices()

    states = _node_get_states()[:]

    b = _rxd_reaction(states) - _diffusion_matrix * states
    

    if not species._has_3d:
        # use Hines solver since 1D only
        states[:] += _reaction_matrix_solve(dt, states, _diffusion_matrix_solve(dt, dt * b))

        # clear the zero-volume "nodes"
        states[_zero_volume_indices] = 0

        # TODO: refactor so this isn't in section1d... probably belongs in node
        _section1d_transfer_to_legacy()
        
        _last_preconditioner = None
    else:
        # TODO: this looks to be semi-implicit method because it doesn't take into account the reaction contribution to the Jacobian; do we care?
        # the actual advance via implicit euler
        n = len(states)
        if _last_dt != dt or _last_preconditioner is None:
            _last_m = eye_minus_dt_J(n, dt)
            _last_preconditioner = _LinearOperator((n, n), _spilu(csc_matrix(_last_m)).solve)
            _last_dt = dt
        # removed diagonal preconditioner since tests showed no improvement in convergence
        result, info = _scipy_sparse_linalg_bicgstab(_last_m, dt * b, M=_last_preconditioner)
        assert(info == 0)
        states[:] += result

        # clear the zero-volume "nodes"
        states[_zero_volume_indices] = 0

        for sr in list(_species_get_all_species().values()):
            s = sr()
            if s is not None: s._transfer_to_legacy()

def _rxd_reaction(states):
    # TODO: this probably shouldn't be here
    # TODO: this was included in the 3d, probably shouldn't be there either
    # TODO: if its None and there is 3D... should we do anything special?
    if _diffusion_matrix is None and not species._has_3d: _setup_matrices()

    b = _numpy_zeros(len(states))
    
    if _curr_ptr_vector is not None:
        _curr_ptr_vector.gather(_curr_ptr_storage_nrn)
        b[_curr_indices] = _curr_scales * (_curr_ptr_storage + _rxd_induced_currents)
    
    #b[_curr_indices] = _curr_scales * [ptr[0] for ptr in _curr_ptrs]

    # TODO: store weak references to the r._evaluate in addition to r so no
    #       repeated lookups
    for rptr in _all_reactions:
        r = rptr()
        if r:
            indices, mult, rate = r._evaluate(states)
            # we split this in parts to allow for multiplicities and to allow stochastic to make the same changes in different places
            for i, m in zip(indices, mult):
                b[i] += m * rate

    node._apply_node_fluxes(b)
    return b
    
_last_preconditioner_dt = 0
_last_dt = None
_last_m = None
_diffusion_d = None
_diffusion_a = None
_diffusion_b = None
_diffusion_p = None
_c_diagonal = None
_cur_node_indices = None

_diffusion_a_ptr, _diffusion_b_ptr, _diffusion_p_ptr = None, None, None

def _diffusion_matrix_solve(dt, rhs):
    # only get here if already initialized
    global _last_dt
    global _diffusion_a_ptr, _diffusion_d, _diffusion_b_ptr, _diffusion_p_ptr, _c_diagonal

    if _diffusion_matrix is None: return numpy.array([])

    n = len(rhs)

    if _last_dt != dt:
        global _c_diagonal, _diffusion_a_base, _diffusion_b_base, _diffusion_d_base
        global _diffusion_a, _diffusion_b, _diffusion_p
        _last_dt = dt
        # clear _c_diagonal and _last_dt to trigger a recalculation
        if _c_diagonal is None:
            _diffusion_d_base = _numpy_array(_diffusion_matrix.diagonal())
            _diffusion_a_base = _numpy_zeros(n)
            _diffusion_b_base = _numpy_zeros(n)
            # TODO: the int32 bit may be machine specific
            _diffusion_p = _numpy_array([-1] * n, dtype=numpy.int32)
            for j in range(n):
                col = _diffusion_matrix[:, j]
                col_nonzero = col.nonzero()
                for i in col_nonzero[0]:
                    if i < j:
                        _diffusion_p[j] = i
                        assert(_diffusion_a_base[j] == 0)
                        _diffusion_a_base[j] = col[i, 0]
                        _diffusion_b_base[j] = _diffusion_matrix[j, i]
            _c_diagonal = _linmodadd_c.diagonal()
        _diffusion_d = _c_diagonal + dt * _diffusion_d_base
        _diffusion_b = dt * _diffusion_b_base
        _diffusion_a = dt * _diffusion_a_base
        _diffusion_a_ptr = _diffusion_a.ctypes.data_as(_double_ptr)
        _diffusion_b_ptr = _diffusion_b.ctypes.data_as(_double_ptr)
        _diffusion_p_ptr = _diffusion_p.ctypes.data_as(_int_ptr)
    
    result = _numpy_array(rhs)
    d = _numpy_array(_diffusion_d)
    d_ptr = d.ctypes.data_as(_double_ptr)
    result_ptr = result.ctypes.data_as(_double_ptr)
    
    nrn_tree_solve(_diffusion_a_ptr, d_ptr, _diffusion_b_ptr, result_ptr,
                   _diffusion_p_ptr, _ctypes_c_int(n))
    return result

def _get_jac(dt, states):
    # only get here if already initialized
    
    # now handle the reaction contribution to the Jacobian
    # this works as long as (I - dt(Jdiff + Jreact)) \approx (I - dtJreact)(I - dtJdiff)
    count = 0
    n = len(states)
    rows = list(range(n))
    cols = list(range(n))
    data = [1] * n
    for rptr in _all_reactions:
        r = rptr()
        if r:
            # TODO: store weakrefs to r._jacobian_entries as well as r
            #       this will reduce lookup time
            r_rows, r_cols, r_data = r._jacobian_entries(states, multiply=-dt)
            # TODO: can we predict the length of rows etc in advance so we
            #       don't need to grow them?
            rows += r_rows
            cols += r_cols
            data += r_data
            count += 1
            
    if count > 0 and n > 0:
        return scipy.sparse.coo_matrix((data, (rows, cols)), shape=(n, n))
    return None

def _reaction_matrix_solve(dt, states, rhs):
    if not options.use_reaction_contribution_to_jacobian:
        return rhs
    jac = _get_jac(dt, states)
    if jac is not None:
        jac = jac.tocsr()
        """
        print 'states:', list(states)
        print 'jacobian (_solve):'
        m = jac.todense()
        for i in xrange(m.shape[0]):
            for j in xrange(m.shape[1]):
                print ('%15g' % m[i, j]),
            print    
        """
        #result, info = scipy.sparse.linalg.bicgstab(jac, rhs)
        #assert(info == 0)
        result = _scipy_sparse_linalg_spsolve(jac, rhs)
    else:
        result = rhs

    return result

_react_matrix_solver = None    
def _reaction_matrix_setup(dt, unexpanded_states):
    global _react_matrix_solver
    if not options.use_reaction_contribution_to_jacobian:
        _react_matrix_solver = lambda x: x
        return

    states = numpy.zeros(len(node._get_states()))
    states[_nonzero_volume_indices] = unexpanded_states    
    jac = _get_jac(dt, states)
    if jac is not None:
        jac = jac.tocsc()
        """
        print 'jacobian (_reaction_matrix_setup):'
        m = jac.todense()
        for i in xrange(m.shape[0]):
            for j in xrange(m.shape[1]):
                print ('%15g' % m[i, j]),
            print
        """
        #result, info = scipy.sparse.linalg.bicgstab(jac, rhs)
        #assert(info == 0)
        _react_matrix_solver = _scipy_sparse_linalg_factorized(jac)
    else:
        _react_matrix_solver = lambda x: x

def _setup():
    initializer._do_init()
    # TODO: this is when I should resetup matrices (structure changed event)
    global _last_dt, _external_solver_initialized
    _last_dt = None
    _external_solver_initialized = False

def _conductance(d):
    pass
    
def _ode_jacobian(dt, t, ypred, fpred):
    #print '_ode_jacobian: dt = %g, last_dt = %r' % (dt, _last_dt)
    lo = _rxd_offset
    hi = lo + len(_nonzero_volume_indices)    
    _reaction_matrix_setup(dt, ypred[lo : hi])

_orig_setup = _setup
_orig_currents = _currents
_orig_ode_count = _ode_count
_orig_ode_reinit = _ode_reinit
_orig_ode_fun = _ode_fun
_orig_ode_solve = _ode_solve
_orig_fixed_step_solve = _fixed_step_solve
_orig_ode_jacobian = _ode_jacobian

# wrapper functions allow swapping in experimental alternatives
def _w_ode_jacobian(dt, t, ypred, fpred): return _ode_jacobian(dt, t, ypred, fpred)
#def _w_conductance(d): return _conductance(d)
_w_conductance = None
def _w_setup(): return _setup()
def _w_currents(rhs): return _currents(rhs)
def _w_ode_count(offset): return _ode_count(offset)
def _w_ode_reinit(y): return _ode_reinit(y)
def _w_ode_fun(t, y, ydot): return _ode_fun(t, y, ydot)
def _w_ode_solve(dt, t, b, y): return _ode_solve(dt, t, b, y)
def _w_fixed_step_solve(raw_dt): return _fixed_step_solve(raw_dt)
def _w_atolscale(y): return _atolscale(y)
_callbacks = [_w_setup, None, _w_currents, _w_conductance, _w_fixed_step_solve,
              _w_ode_count, _w_ode_reinit, _w_ode_fun, _w_ode_solve, _w_ode_jacobian, _w_atolscale]

_curr_ptr_vector = None
_curr_ptr_storage = None
_curr_ptr_storage_nrn = None
pinverse = None
_cur_map = None
_h_ptrvector = h.PtrVector
_h_vector = h.Vector

_structure_change_count = neuron.nrn_dll_sym('structure_change_cnt', _ctypes_c_int)
_diam_change_count = neuron.nrn_dll_sym('diam_change_cnt', _ctypes_c_int)

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
        for sr in list(_species_get_all_species().values()):
            s = sr()
            if s is not None: s._update_node_data()
        for sr in list(_species_get_all_species().values()):
            s = sr()
            if s is not None: s._update_region_indices()
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

        _curr_scales = _numpy_array(_curr_scales)        
        

_euler_matrix = None

# TODO: make sure this does the right thing when the diffusion constant changes between two neighboring nodes
def _setup_matrices():
    global _linmodadd, _linmodadd_c, _diffusion_matrix, _linmodadd_b, _last_dt, _c_diagonal, _euler_matrix
    global _cur_node_indices
    global _zero_volume_indices, _nonzero_volume_indices

    # TODO: this sometimes seems to get called twice. Figure out why and fix, if possible.

    n = len(_node_get_states())
        
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
        

    if species._has_1d:
        n = species._1d_submatrix_n()
    
        # TODO: initialization is slow. track down why
        
        _last_dt = None
        _c_diagonal = None
        
        for sr in list(_species_get_all_species().values()):
            s = sr()
            if s is not None:
                s._assign_parents()
        
        _update_node_data(True)

        # remove old linearmodeladdition
        _linmodadd = None
        _linmodadd_cur = None
        
        if n:        
            # create sparse matrix for C in cy'+gy=b
            _linmodadd_c = _scipy_sparse_dok_matrix((n, n))
            # most entries are 1 except those corresponding to the 0 and 1 ends
            
            
            # create the matrix G
            if not species._has_3d:
                # if we have both, then put the 1D stuff into the matrix that already exists for 3D
                _diffusion_matrix = _scipy_sparse_dok_matrix((n, n))
            for sr in list(_species_get_all_species().values()):
                s = sr()
                if s is not None:
                    #print '_diffusion_matrix.shape = %r, n = %r, species._has_3d = %r' % (_diffusion_matrix.shape, n, species._has_3d)
                    s._setup_diffusion_matrix(_diffusion_matrix)
                    s._setup_c_matrix(_linmodadd_c)
            
            # modify C for cases where no diffusive coupling of 0, 1 ends
            # TODO: is there a better way to handle no diffusion?
            for i in range(n):
                if not _diffusion_matrix[i, i]:
                    _linmodadd_c[i, i] = 1

            
            # and the vector b
            _linmodadd_b = _h_vector(n)

            
            # setup for induced membrane currents
            _cur_node_indices = []

            for rptr in _all_reactions:
                r = rptr()
                if r is not None:
                    r._setup_membrane_fluxes(_cur_node_indices, _cur_map)
                    
            #_cvode_object.re_init()    

            _linmodadd_c = _linmodadd_c.tocsr()
            
            if species._has_3d:
                _euler_matrix = -_diffusion_matrix
            
        volumes = node._get_data()[0]
        _zero_volume_indices = numpy.where(volumes == 0)[0]
        _nonzero_volume_indices = volumes.nonzero()[0]


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
                            parent_sec = morphology.parent(sec)
                            # are any of these a match with a 1d section?
                            if s._has_region_section(r, parent_sec):
                                # this section has a 1d section that is a parent
                                index1d, indices3d = _get_node_indices(s, r, sec, h.section_orientation(sec=sec), parent_sec, h.parent_connection(sec=sec))
                                hybrid_neighbors[index1d] += indices3d
                                hybrid_diams[index1d] = parent_sec(h.parent_connection(sec=sec)).diam
                            else:
                                for sec1d in r._secs1d:
                                    parent_1d = morphology.parent(sec1d)
                                    if parent_1d == sec:
                                        # it is the parent of a 1d section
                                        index1d, indices3d = _get_node_indices(s, r, sec, h.parent_connection(sec=sec1d), sec1d, h.section_orientation(sec=sec1d))
                                        hybrid_neighbors[index1d] += indices3d
                                        hybrid_diams[index1d] = sec1d(h.section_orientation(sec=sec1d)).diam
                                        break
                                    elif parent_1d == parent_sec:
                                        # it connects to the parent of a 1d section
                                        index1d, indices3d = _get_node_indices(s, r, sec, h.section_orientation(sec=sec), sec1d, h.section_orientation(sec=sec1d))
                                        hybrid_neighbors[index1d] += indices3d
                                        hybrid_diams[index1d] = sec1d(h.section_orientation(sec=sec1d)).diam
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
    
    # we do this last because of performance issues with changing sparsity of csr matrices
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
    if x1d == h.section_orientation(sec=sec1d):
        # TODO: make this whole thing more efficient
        # the parent node is the nonzero index on the first row before the diagonal
        first_row = min([node._index for node in species.nodes(region)(sec1d)])
        for j in range(first_row):
            if _euler_matrix[first_row, j] != 0:
                index_1d = j
                break
        else:
            raise RxDException('should never get here; could not find parent')
    elif x1d == 1 - h.section_orientation(sec=sec1d):
        # the ending zero-volume node is the one after the last node
        # TODO: make this more efficient
        index_1d = max([node._index for node in species.nodes(region)(sec1d)]) + 1
    else:
        raise RxDException('should never get here; _get_node_indices apparently only partly converted to allow connecting to 1d in middle')
    #print '1d index is %d' % index_1d
    return index_1d, indices3d
    
def _init():
    initializer._do_init()
    
    # TODO: check about the 0<x<1 problem alluded to in the documentation
    h.define_shape()
    
    if species._has_1d:
        section1d._purge_cptrs()
    
    for sr in list(_species_get_all_species().values()):
        s = sr()
        if s is not None:
            # TODO: are there issues with hybrid or 3D here? (I don't think so, but here's a bookmark just in case)
            s._register_cptrs()
            s._finitialize()
    _setup_matrices()

_has_nbs_registered = False
_nbs = None
def _do_nbs_register():
    global _has_nbs_registered, _nbs, _fih, _fih2
    
    if not _has_nbs_registered:
        from neuron import nonvint_block_supervisor as _nbs

        _has_nbs_registered = True
        _nbs.register(_callbacks)
    
        #
        # register the initialization handler and the ion register handler
        #
        _fih = h.FInitializeHandler(_init)
        _fih2 = h.FInitializeHandler(3, initializer._do_ion_register)

        #
        # register scatter/gather mechanisms
        #
        _cvode_object.extra_scatter_gather(0, _after_advance)


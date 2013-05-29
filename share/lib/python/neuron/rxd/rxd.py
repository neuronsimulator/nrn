import neuron
from neuron import h
import species
from nodelist import NodeList
import node
import section1d
import weakref
import numpy
from neuron import nonvint_block_supervisor as nbs
import scipy.sparse
import scipy.sparse.linalg
import ctypes
import atexit
import options
import region

def byeworld():
    # needed to prevent a seg-fault error at shudown in at least some
    # combinations of NEURON and Python, which I think is due to objects
    # getting deleted out-of-order
    global _react_matrix_solver
    del _react_matrix_solver
    
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
_int_ptr = ctypes.POINTER(ctypes.c_int)


_nrn_dll = neuron.nrn_dll()
nrn_tree_solve = _nrn_dll.nrn_tree_solve
nrn_tree_solve.restype = None

_dptr = _double_ptr

def _unregister_reaction(r):
    global _all_reactions
    for i, r2 in enumerate(_all_reactions):
        if r2() == r:
            del _all_reactions[i]
            break

def _register_reaction(r):
    # TODO: should we search to make sure that (a weakref to) r hasn't already been added?
    global _all_reactions
    _all_reactions.append(weakref.ref(r))

def _after_advance():
    global last_diam_change_cnt
    last_diam_change_cnt = _cvode_object.diam_change_count()
    
def re_init():
    """reinitializes all rxd concentrations to match HOC values, updates matrices"""
    h.define_shape()
    
    dim = region._sim_dimension
    
    if dim == 1:
        # update current pointers
        section1d._purge_cptrs()
        for sr in species._get_all_species().values():
            s = sr()
            if s is not None:
                s._register_cptrs()
        
        # update matrix equations
        _setup_matrices()

    elif dim != 3:            
        raise Exception('unknown dimension')
    for sr in species._get_all_species().values():
        s = sr()
        if s is not None: s.re_init()
    # TODO: is this safe?        
    _cvode_object.re_init()


def _invalidate_matrices():
    # TODO: make a separate variable for this?
    global last_structure_change_cnt, _diffusion_matrix
    _diffusion_matrix = None
    last_structure_change_cnt = None

_rxd_offset = None

def _ode_count(offset):
    global _rxd_offset
    _rxd_offset = offset
    if _diffusion_matrix is None: _setup_matrices()
    return len(_nonzero_volume_indices)

def _ode_reinit(y):
    if region._sim_dimension == 3: raise Exception('cvode not supported yet for 3D')
    y[_rxd_offset : _rxd_offset + len(_nonzero_volume_indices)] = node._get_states()[_nonzero_volume_indices]

def _ode_fun(t, y, ydot):
    current_dimension = region._sim_dimension
    if region._sim_dimension == 3: raise Exception('cvode not supported yet for 3D')
    lo = _rxd_offset
    hi = lo + len(_nonzero_volume_indices)
    if lo == hi: return
    states = node._get_states()
    states[_nonzero_volume_indices] = y[lo : hi]

    # need to fill in the zero volume states with the correct concentration
    # this assumes that states at the zero volume indices is zero (although that
    # assumption could be easily removed)
    #matrix = scipy.sparse.dok_matrix((len(_zero_volume_indices), len(states)))
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
    states[_zero_volume_indices] = _mat_for_zero_volume_nodes * states
    """
    for i in _zero_volume_indices:
        v = _diffusion_matrix[i] * states
        d = _diffusion_matrix[i, i]
        if d:
            states[i] = -v / d
    """
    if region._sim_dimension == 1:
        # TODO: refactor so this isn't in section1d?
        section1d._transfer_to_legacy()        
    elif region._sim_dimension == 3:
        for sr in species._get_all_species().values():
            s = sr()
            if s is not None: s._transfer_to_legacy()
    else:
        raise Exception('unknown dimension')

    
    if ydot is not None:
        # diffusion_matrix = - jacobian    
        ydot[lo : hi] = (_rxd_reaction(states) - _diffusion_matrix * states)[_nonzero_volume_indices]
        
    states[_zero_volume_indices] = 0

def _ode_solve(dt, t, b, y):
    if _diffusion_matrix is None: _setup_matrices()
    lo = _rxd_offset
    hi = lo + len(_nonzero_volume_indices)
    n = len(node._get_states())
    full_b = numpy.zeros(n)
    full_b[_nonzero_volume_indices] = b[lo : hi]
    b[lo : hi] = _react_matrix_solver(_diffusion_matrix_solve(dt, full_b))[_nonzero_volume_indices]
    # this line doesn't include the reaction contributions to the Jacobian
    #b[lo : hi] = _diffusion_matrix_solve(dt, full_b)[_nonzero_volume_indices]


_rxd_induced_currents = None

def _currents(rhs):
    # setup membrane fluxes from our stuff
    # TODO: cache the memb_cur_ptrs, memb_cur_charges, memb_net_charges, memb_cur_mapped
    #       because won't change very often
    global _rxd_induced_currents

    # need this; think it's because of initialization of mod files
    if _curr_indices is None: return

    # TODO: change so that this is only called when there are in fact currents
    _rxd_induced_currents = numpy.zeros(len(_curr_indices))
    rxd_memb_flux = []
    memb_cur_ptrs = []
    memb_cur_charges = []
    memb_net_charges = []
    memb_cur_mapped = []
    for rptr in _all_reactions:
        r = rptr()
        if r and r._membrane_flux:
            # NOTE: memb_flux contains any scaling we need
            new_fluxes = r._get_memb_flux(node._get_states())
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

preconditioner = None
def _fixed_step_solve(dt):
    global preconditioner, pinverse

    states = node._get_states()[:]

    b = _rxd_reaction(states)
    
    dim = region._sim_dimension
    if dim is None:
        return
    elif dim == 1:
        # TODO (major): is this right? I think _ode_solve does it right and this is wrong
        
        # the solve is logically equivalent to the commented out line, but simpler
        #states[:] += _diffusion_matrix_solve(dt, -dt * _diffusion_matrix * states)
        states[:] = _diffusion_matrix_solve(dt, states)
        states[:] += _reaction_matrix_solve(dt, dt * b)

        # clear the zero-volume "nodes"
        states[_zero_volume_indices] = 0

        # TODO: refactor so this isn't in section1d... probably belongs in node
        section1d._transfer_to_legacy()
    elif dim == 3:
        # the actual advance via implicit euler
        n = len(states)
        m = scipy.sparse.eye(n, n) - dt * _euler_matrix
        # TODO: pinverse should be updated whenever dt changes
        if pinverse is not None:
            pinverse = scipy.sparse.diags([[1. / m.diagonal()]], [0], shape=(n, n), format='csr', dtype='d')
        #result, info = scipy.sparse.linalg.bicgstab(m, states + dt * b, M=pinverse)
        result, info = scipy.sparse.linalg.bicgstab(m, states, M=pinverse)
        result += dt * b
        assert(info == 0)
        states[:] = result

        for sr in species._get_all_species().values():
            s = sr()
            if s is not None: s._transfer_to_legacy()
        

def _rxd_reaction(states):
    # TODO: this probably shouldn't be here
    # TODO: this was included in the 3d, probably shouldn't be there either
    if _diffusion_matrix is None and region._sim_dimension == 1: _setup_matrices()

    b = numpy.zeros(len(states))
    
    if _curr_ptr_vector is not None:
        _curr_ptr_vector.gather(_curr_ptr_storage_nrn)
        b[_curr_indices] = _curr_scales * (_curr_ptr_storage + _rxd_induced_currents)
    
    #b[_curr_indices] = _curr_scales * [ptr[0] for ptr in _curr_ptrs]

    for rptr in _all_reactions:
        r = rptr()
        if r:
            indices, mult, rate = r._evaluate(states)
            # we split this in parts to allow for multiplicities and to allow stochastic to make the same changes in different places
            for i, m in zip(indices, mult):
                b[i] += m * rate


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
    global _last_dt
    global _diffusion_a_ptr, _diffusion_d, _diffusion_b_ptr, _diffusion_p_ptr, _c_diagonal

    if _diffusion_matrix is None: return

    n = len(rhs)

    if _last_dt != dt:
        global _c_diagonal, _diffusion_a_base, _diffusion_b_base, _diffusion_d_base
        global _diffusion_a, _diffusion_b, _diffusion_p
        _last_dt = dt
        # clear _c_diagonal and _last_dt to trigger a recalculation
        if _c_diagonal is None:
            _diffusion_d_base = numpy.array(_diffusion_matrix.diagonal())
            _diffusion_a_base = numpy.zeros(n)
            _diffusion_b_base = numpy.zeros(n)
            # TODO: the int32 bit may be machine specific
            _diffusion_p = numpy.array([-1] * n, dtype=numpy.int32)
            for j in xrange(n):
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
    
    result = numpy.array(rhs)
    d = numpy.array(_diffusion_d)
    d_ptr = d.ctypes.data_as(_double_ptr)
    result_ptr = result.ctypes.data_as(_double_ptr)
    
    nrn_tree_solve(_diffusion_a_ptr, d_ptr, _diffusion_b_ptr, result_ptr,
                   _diffusion_p_ptr, ctypes.c_int(n))
    return result

def _reaction_matrix_solve(dt, rhs):
    if not options.use_reaction_contribution_to_jacobian:
        return rhs
    # now handle the reaction contribution to the Jacobian
    # this works as long as (I - dt(Jdiff + Jreact)) \approx (I - dtJreact)(I - dtJdiff)
    count = 0
    n = len(rhs)
    rows = range(n)
    cols = range(n)
    data = [1] * n
    for rptr in _all_reactions:
        r = rptr()
        if r:
            r_rows, r_cols, r_data = r._jacobian_entries(rhs, multiply=-dt)
            rows += r_rows
            cols += r_cols
            data += r_data
            count += 1
            
    if count > 0:
        jac = scipy.sparse.coo_matrix((data, (rows, cols)), shape=(n, n)).tocsr()
        #result, info = scipy.sparse.linalg.bicgstab(jac, rhs)
        #assert(info == 0)
        result = scipy.sparse.linalg.spsolve(jac, rhs)
    else:
        result = rhs

    return result

_react_matrix_solver = None    
def _reaction_matrix_setup(dt, rhs):
    global _react_matrix_solver
    if not options.use_reaction_contribution_to_jacobian:
        _react_matrix_solver = lambda x: x
        return

    # now handle the reaction contribution to the Jacobian
    # this works as long as (I - dt(Jdiff + Jreact)) \approx (I - dtJreact)(I - dtJdiff)
    count = 0
    n = len(rhs)
    rows = range(n)
    cols = range(n)
    data = [1] * n
    for rptr in _all_reactions:
        r = rptr()
        if r:
            r_rows, r_cols, r_data = r._jacobian_entries(rhs, multiply=-dt)
            rows += r_rows
            cols += r_cols
            data += r_data
            count += 1
            
    if count > 0:
        
        jac = scipy.sparse.coo_matrix((data, (rows, cols)), shape=(n, n)).tocsc()
        #result, info = scipy.sparse.linalg.bicgstab(jac, rhs)
        #assert(info == 0)
        _react_matrix_solver = scipy.sparse.linalg.factorized(jac)
    else:
        _react_matrix_solver = lambda x: x

def _setup():
    # TODO: this is when I should resetup matrices (structure changed event)
    pass

def _conductance(d):
    pass
    
def _ode_jacobian(dt, t, ypred, fpred):
    #print '_ode_jacobian: dt = %g, last_dt = %r' % (dt, _last_dt)
    _reaction_matrix_setup(dt, fpred)

_callbacks = [_setup, None, _currents, _conductance, _fixed_step_solve,
              _ode_count, _ode_reinit, _ode_fun, _ode_solve, _ode_jacobian, None]
nbs.register(_callbacks)

_curr_ptr_vector = None
_curr_ptr_storage = None
_curr_ptr_storage_nrn = None
pinverse = None
_cur_map = None

def _update_node_data(force=False):
    global last_diam_change_cnt, last_structure_change_cnt, _curr_indices, _curr_scales, _curr_ptrs, _cur_map
    global _curr_ptr_vector, _curr_ptr_storage, _curr_ptr_storage_nrn
    if last_diam_change_cnt != _cvode_object.diam_change_count() or _cvode_object.structure_change_count() != last_structure_change_cnt or force:
        _cur_map = {}
        last_diam_change_cnt = _cvode_object.diam_change_count()
        last_structure_change_cnt = _cvode_object.structure_change_count()
        if region._sim_dimension == 1:
            # TODO: merge this with the 3d case
            for sr in species._get_all_species().values():
                s = sr()
                if s is not None: s._update_node_data()
            for sr in species._get_all_species().values():
                s = sr()
                if s is not None: s._update_region_indices()
        for rptr in _all_reactions:
            r = rptr()
            if r is not None: r._update_indices()
        _curr_indices = []
        _curr_scales = []
        _curr_ptrs = []
        for sr in species._get_all_species().values():
            s = sr()
            if s is not None: s._setup_currents(_curr_indices, _curr_scales, _curr_ptrs, _cur_map)
        
        num = len(_curr_ptrs)
        if num:
            _curr_ptr_vector = h.PtrVector(num)
            for i, ptr in enumerate(_curr_ptrs):
                _curr_ptr_vector.pset(i, ptr)
            
            _curr_ptr_storage_nrn = h.Vector(num)
            _curr_ptr_storage = _curr_ptr_storage_nrn.as_numpy()
        else:
            _curr_ptr_vector = None

        _curr_scales = numpy.array(_curr_scales)        
        

_euler_matrix = None

# TODO: make sure this does the right thing when the diffusion constant changes between two neighboring nodes
def _setup_matrices():
    global _linmodadd, _linmodadd_c, _diffusion_matrix, _linmodadd_b, _last_dt, _c_diagonal, _euler_matrix
    global _cur_node_indices


    n = len(node._get_states())
        
    if region._sim_dimension == 3:
        _euler_matrix = scipy.sparse.dok_matrix((n, n), dtype=float)

        for sr in species._get_all_species().values():
            s = sr()
            if s is not None: s._setup_matrices3d(_euler_matrix)

        _euler_matrix = _euler_matrix.tocsr()

        _update_node_data(True)
        

    else:
        # TODO: initialization is slow. track down why
        
        _last_dt = None
        _c_diagonal = None
        
        for sr in species._get_all_species().values():
            s = sr()
            if s is not None:
                s._assign_parents()
        
        _update_node_data(True)

        # remove old linearmodeladdition
        _linmodadd = None
        _linmodadd_cur = None
        
        if n:        
            # create sparse matrix for C in cy'+gy=b
            _linmodadd_c = scipy.sparse.dok_matrix((n, n))
            # most entries are 1 except those corresponding to the 0 and 1 ends
            
            
            # create the matrix G
            _diffusion_matrix = scipy.sparse.dok_matrix((n, n))
            for sr in species._get_all_species().values():
                s = sr()
                if s is not None:
                    s._setup_diffusion_matrix(_diffusion_matrix)
                    s._setup_c_matrix(_linmodadd_c)
            
            # modify C for cases where no diffusive coupling of 0, 1 ends
            # TODO: is there a better way to handle no diffusion?
            for i in xrange(n):
                if not _diffusion_matrix[i, i]:
                    _linmodadd_c[i, i] = 1

            
            # and the vector b
            _linmodadd_b = h.Vector(n)

            
            # setup for induced membrane currents
            _cur_node_indices = []

            for rptr in _all_reactions:
                r = rptr()
                if r is not None:
                    r._setup_membrane_fluxes(_cur_node_indices, _cur_map)
                    
            #_cvode_object.re_init()    

            _linmodadd_c = _linmodadd_c.tocsr()
            _diffusion_matrix = _diffusion_matrix.tocsr()
            
        global _zero_volume_indices, _nonzero_volume_indices
        volumes = node._get_data()[0]
        _zero_volume_indices = numpy.where(volumes == 0)[0]
        _nonzero_volume_indices = volumes.nonzero()[0]

        if n:
            matrix = _diffusion_matrix[_zero_volume_indices].todok()
            for row, i in enumerate(_zero_volume_indices):
                d = _diffusion_matrix[i, i]
                if d:
                    matrix[row, :] /= -d
                    matrix[row, i] = 0
                else:
                    matrix[row, :] = 0
            global _mat_for_zero_volume_nodes
            _mat_for_zero_volume_nodes = matrix.tocsr()


def _init():
    # TODO: check about the 0<x<1 problem alluded to in the documentation
    h.define_shape()
    
    
    dim = region._sim_dimension
    if dim == 3:
        for sr in species._get_all_species().values():
            s = sr()
            if s is not None:
                # s._register_cptrs()
                s._finitialize()
        _setup_matrices()
    elif dim == 1:
        section1d._purge_cptrs()
        
        for sr in species._get_all_species().values():
            s = sr()
            if s is not None:
                s._register_cptrs()
                s._finitialize()
        _setup_matrices()

#
# register the initialization handler and the advance handler
#
_fih = h.FInitializeHandler(_init)

#
# register scatter/gather mechanisms
#
_cvode_object.extra_scatter_gather(0, _after_advance)


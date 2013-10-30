import neuron
from neuron import h
from . import species, node, section1d
from .nodelist import NodeList
import weakref
import numpy
from neuron import nonvint_block_supervisor as nbs
import scipy.sparse
import scipy.sparse.linalg
import ctypes
import atexit
import options

def byeworld():
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

_linmodadd_cur = None
_linmodadd_cur_c = None
_linmodadd_cur_g = None
_linmodadd_cur_b = None
_linmodadd_cur_y = None
_cur_sec_list = None
_cur_x_list = None

_all_reactions = []

_zero_volume_indices = []
_nonzero_volume_indices = []

_double_ptr = ctypes.POINTER(ctypes.c_double)
_int_ptr = ctypes.POINTER(ctypes.c_int)


nrn_tree_solve = neuron.nrn_dll_sym('nrn_tree_solve')
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
    
    # update current pointers
    section1d._purge_cptrs()
    for sr in species._get_all_species().values():
        s = sr()
        if s is not None:
            s._register_cptrs()
    
    # update matrix equations
    _setup_matrices()
        
    for sr in species._get_all_species().values():
        s = sr()
        if s is not None: s.re_init()
    
    _cvode_object.re_init()

def _invalidate_matrices():
    # TODO: make a separate variable for this?
    global last_structure_change_cnt, _diffusion_matrix
    _diffusion_matrix = None
    last_structure_change_cnt = None

def _setup_output_flux_ptrs():
    global _output_curr_valences, _output_curr_ptrs
    _output_curr_valences = []
    _output_curr_ptrs = []
    states = numpy.array(node._get_states())
    for rptr in _all_reactions:
        r = rptr()
        if r:
            memb_flux = r._get_memb_flux(states)
            if memb_flux:
                # TODO: check sign; might be reversed
                for sign, sources in zip([1, -1], [r._sources, r._dests]):
                    for source in sources:
                        name = '_ref_i%s' % (source._species().name)
                        charge = source._species().charge
                        for sec in r._regions[0].secs:
                            for i in xrange(sec.nseg):
                                _output_curr_ptrs.append(sec._sec((i + 0.5) / sec.nseg).__getattribute__(name))
                            _output_curr_valences += [charge * sign] * sec.nseg
    _output_curr_valences = numpy.array(_output_curr_valences)

_rxd_offset = None

def _ode_count(offset):
    global _rxd_offset
    _rxd_offset = offset
    if _diffusion_matrix is None: _setup_matrices()
    return len(_nonzero_volume_indices)

def _ode_reinit(y):
    y[_rxd_offset : _rxd_offset + len(_nonzero_volume_indices)] = node._get_states()[_nonzero_volume_indices]

def _ode_fun(t, y, ydot):
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
    # TODO: refactor so this isn't in section1d
    section1d._transfer_to_legacy()        
    
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


def _fixed_step_currents(rhs):
    # setup membrane fluxes from our stuff
    rxd_memb_flux = []
    for rptr in _all_reactions:
        r = rptr()
        if r and r._membrane_flux:
            rxd_memb_flux += list(r._get_memb_flux(states)) * (len(r._sources) + len(r._dests))
            raise Exception('rxd_memb_flux not yet supported')

    if rxd_memb_flux:
        _linmodadd_cur_b.from_python(rxd_memb_flux)
        rxd_memb_flux = _output_curr_valences * rxd_memb_flux
        for ptr, cur in zip(_output_curr_ptrs, rxd_memb_flux):
            ptr[0] += cur


preconditioner = None
def _fixed_step_solve(dt):
    global preconditioner

    # TODO: this probably shouldn't be here
    if _diffusion_matrix is None: _setup_matrices()

    # TODO: use linear approx not constant approx
    states = node._get_states()[:]

    b = _rxd_reaction(states) - _diffusion_matrix * states
    
    states[:] += _reaction_matrix_solve(dt, _diffusion_matrix_solve(dt, dt * b))

    # clear the zero-volume "nodes"
    states[_zero_volume_indices] = 0

    # TODO: refactor so this isn't in section1d... probably belongs in node
    section1d._transfer_to_legacy()

def _rxd_reaction(states):
    # TODO: this probably shouldn't be here
    # TODO: this was included in the 3d, probably shouldn't be there either
    if _diffusion_matrix is None: _setup_matrices()

    b = numpy.zeros(len(states))
    # TODO: this isn't yet (2013-03-14) in 3D, but needs to be
    # membrane fluxes from classic NEURON
    #print 'indices:', _curr_indices
    #print 'ptrs:', _curr_ptrs
    
    if _curr_ptr_vector is not None:
        _curr_ptr_vector.gather(_curr_ptr_storage_nrn)
        b[_curr_indices] = _curr_scales * _curr_ptr_storage
    
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

_diffusion_a_ptr, _diffusion_b_ptr, _diffusion_p_ptr = None, None, None

def _diffusion_matrix_solve(dt, rhs):
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
            
    if count > 0 and n > 0:
        jac = scipy.sparse.coo_matrix((data, (rows, cols)), shape=(n, n)).tocsr()
        #result, info = scipy.sparse.linalg.bicgstab(jac, rhs)
        #assert(info == 0)
        result = scipy.sparse.linalg.spsolve(jac, rhs)
    else:
        result = rhs

    return result

_react_matrix_solver = None    
def _reaction_matrix_setup(dt, unexpanded_rhs):
    global _react_matrix_solver
    if not options.use_reaction_contribution_to_jacobian:
        _react_matrix_solver = lambda x: x
        return

    # now handle the reaction contribution to the Jacobian
    # this works as long as (I - dt(Jdiff + Jreact)) \approx (I - dtJreact)(I - dtJdiff)
    count = 0
    rhs = numpy.array(node._get_states())
    rhs[_nonzero_volume_indices] = unexpanded_rhs    
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

_callbacks = [_setup, None, _fixed_step_currents, _conductance, _fixed_step_solve,
              _ode_count, _ode_reinit, _ode_fun, _ode_solve, _ode_jacobian, None]
nbs.register(_callbacks)

_curr_ptr_vector = None
_curr_ptr_storage = None
_curr_ptr_storage_nrn = None

def _update_node_data(force=False):
    global last_diam_change_cnt, last_structure_change_cnt, _curr_indices, _curr_scales, _curr_ptrs
    global _curr_ptr_vector, _curr_ptr_storage, _curr_ptr_storage_nrn
    if last_diam_change_cnt != _cvode_object.diam_change_count() or _cvode_object.structure_change_count() != last_structure_change_cnt or force:
        last_diam_change_cnt = _cvode_object.diam_change_count()
        last_structure_change_cnt = _cvode_object.structure_change_count()
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
            if s is not None: s._setup_currents(_curr_indices, _curr_scales, _curr_ptrs)
        
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
        

# TODO: make sure this does the right thing when the diffusion constant changes between two neighboring nodes
def _setup_matrices():
    global _linmodadd, _linmodadd_c, _diffusion_matrix, _linmodadd_b, _last_dt, _c_diagonal
    global _linmodadd_cur, _linmodadd_cur_c, _linmodadd_cur_g, _linmodadd_cur_b, _cur_sec_list, _cur_x_list, _linmodadd_cur_y
    
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
    
    n = len(node._get_states())
    
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

        #print 'c:'
        #for i in xrange(n):
        #    print ' '.join('%10g' % _linmodadd_c.getval(i, j) for j in xrange(n))

        
        #print 'g:'
        #for i in xrange(n):
        #    print ' '.join('%10g' % _diffusion_matrix.getval(i, j) for j in xrange(n))
                
        
        # and finally register everything with NEURON
        #_linmodadd = h.LinearMechanism(_linmodadd_c, _diffusion_matrix, node._get_states(), node._get_states(), _linmodadd_b)
        
        # add the LinearMechanism for the membrane fluxes (if any)
        _cur_sec_list = h.SectionList()
        _cur_x_list = h.Vector()

        for rptr in _all_reactions:
            r = rptr()
            if r is not None:
                r._setup_membrane_fluxes(_cur_sec_list, _cur_x_list)
        
        cur_size = _cur_x_list.size()
        if cur_size:
            raise Exception('membrane fluxes not yet supported')
            _linmodadd_cur_b = h.Vector(cur_size)
            _linmodadd_cur_c = scipy.sparse.dok_matrix((cur_size, cur_size))
            _linmodadd_cur_g = scipy.sparse.dok_matrix((cur_size, cur_size))
            # this gets copies of membrane potentials; don't currently use them
            # except in that LinearMechanism requires a place to store them
            _linmodadd_cur_y = h.Vector(cur_size)
            _linmodadd_cur = h.LinearMechanism(_linmodadd_cur_c, _linmodadd_cur_g, _linmodadd_cur_y, _linmodadd_cur_b, _cur_sec_list, _cur_x_list)
        else:
            _linmodadd_cur = None
    
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


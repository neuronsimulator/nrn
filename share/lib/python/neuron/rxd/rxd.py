import neuron
from neuron import h
import species
from nodelist import NodeList
import node
import section1d
import weakref
import numpy

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
_linmodadd_g = None
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

def _advance2():
    global last_diam_change_cnt
    # TODO: refactor so this isn't in section1d
    section1d._transfer_to_legacy()
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
    global last_structure_change_cnt
    last_structure_change_cnt = None

def _do_imports():
    # TODO: is this really what I want? need it to keep cvode from misbehaving
    if _cvode_object.active(): return
    global last_diam_change_cnt, last_structure_change_cnt, _linmodadd_b

    for sr in species._get_all_species().values():
        s = sr()
        if s is not None: s._import_concentration(init=False)

    if last_diam_change_cnt != _cvode_object.diam_change_count() or _cvode_object.structure_change_count() != last_structure_change_cnt:
        # TODO: if last_structure_change_cnt is out of date, possibility that
        #        recreated nodes might not be correct; should force regeneration
        #        from section objects AND make sure that their state values are
        #        preserved
        _setup_matrices()
        last_structure_change_cnt = _cvode_object.structure_change_count()
        section1d._purge_cptrs()
        _setup_output_flux_ptrs()

        for sr in species._get_all_species().values():
            s = sr()
            if s is not None: s._register_cptrs()

        _cvode_object.re_init()


def _setup_output_flux_ptrs():
    global _output_curr_valences, _output_curr_ptrs
    _output_curr_valences = []
    _output_curr_ptrs = []
    states = numpy.array(node._get_states().to_python())
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


def _advance():
    global last_diam_change_cnt, last_structure_change_cnt, _linmodadd_b
    
    # TODO: use linear approx not constant approx (i.e. update _linmodadd_g as well; right now effectively treating reactions via forward euler)
    states = numpy.array(node._get_states().to_python())

    b = 0 * states

    for rptr in _all_reactions:
        r = rptr()
        if r:
            indices, mult, rate = r._evaluate(states)
            # we split this in parts to allow for multiplicities and to allow stochastic to make the same changes in different places
            for i, m in zip(indices, mult):
                b[i] += m * rate
    
    # membrane fluxes from classic NEURON
    b[_curr_indices] += _curr_scales * [ptr[0] for ptr in _curr_ptrs]
    
    # setup membrane fluxes from our stuff
    rxd_memb_flux = []
    for rptr in _all_reactions:
        r = rptr()
        if r and r._membrane_flux:
            rxd_memb_flux += list(r._get_memb_flux(states)) * (len(r._sources) + len(r._dests))

    # copy it into classic NEURON
    _linmodadd_b.from_python(b)
    if rxd_memb_flux:
        _linmodadd_cur_b.from_python(rxd_memb_flux)
        rxd_memb_flux = _output_curr_valences * rxd_memb_flux
        for ptr, cur in zip(_output_curr_ptrs, rxd_memb_flux):
            ptr[0] += cur
            
        



def _update_node_data(force=False):
    global last_diam_change_cnt, last_structure_change_cnt, _curr_indices, _curr_scales, _curr_ptrs
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
        _curr_scales = numpy.array(_curr_scales)        
        

# TODO: make sure this does the right thing when the diffusion constant changes between two neighboring nodes
def _setup_matrices():
    global _linmodadd, _linmodadd_c, _linmodadd_g, _linmodadd_b
    global _linmodadd_cur, _linmodadd_cur_c, _linmodadd_cur_g, _linmodadd_cur_b, _cur_sec_list, _cur_x_list, _linmodadd_cur_y
    for sr in species._get_all_species().values():
        s = sr()
        if s is not None:
            s._assign_parents()
    
    _update_node_data(True)

    # remove old linearmodeladdition
    _linmodadd = None
    _linmodadd_cur = None
    
    n = int(node._get_states().size())
    
    if n:        
        # create sparse matrix for C in cy'+gy=b
        _linmodadd_c = h.Matrix(n, n, 2)
        # most entries are 1 except those corresponding to the 0 and 1 ends
        
        
        # create the matrix G
        _linmodadd_g = h.Matrix(n, n, 2)
        for sr in species._get_all_species().values():
            s = sr()
            if s is not None:
                s._setup_diffusion_matrix(_linmodadd_g)
                s._setup_c_matrix(_linmodadd_c)
        
        # modify C for cases where no diffusive coupling of 0, 1 ends
        # TODO: is there a better way to handle no diffusion?
        for i in xrange(n):
            if not _linmodadd_g.getval(i, i):
                _linmodadd_c.setval(i, i, 1)

        
        # and the vector b
        _linmodadd_b = h.Vector(n)

        #print 'c:'
        #for i in xrange(n):
        #    print ' '.join('%10g' % _linmodadd_c.getval(i, j) for j in xrange(n))

        
        #print 'g:'
        #for i in xrange(n):
        #    print ' '.join('%10g' % _linmodadd_g.getval(i, j) for j in xrange(n))
                
        
        # and finally register everything with NEURON

        _linmodadd = h.LinearMechanism(_advance, _linmodadd_c, _linmodadd_g, node._get_states(), node._get_states(), _linmodadd_b)
        #_linmodadd = h.LinearMechanism(_linmodadd_c, _linmodadd_g, node._get_states(), node._get_states(), _linmodadd_b)
        
        # add the LinearMechanism for the membrane fluxes (if any)
        _cur_sec_list = h.SectionList()
        _cur_x_list = h.Vector()

        for rptr in _all_reactions:
            r = rptr()
            if r is not None: r._setup_membrane_fluxes(_cur_sec_list, _cur_x_list)
        
        cur_size = _cur_x_list.size()
        if cur_size:
            _linmodadd_cur_b = h.Vector(cur_size)
            _linmodadd_cur_c = h.Matrix(cur_size, cur_size, 2)
            _linmodadd_cur_g = h.Matrix(cur_size, cur_size, 2)
            # this gets copies of membrane potentials; don't currently use them
            # except in that LinearMechanism requires a place to store them
            _linmodadd_cur_y = h.Vector(cur_size)
            _linmodadd_cur = h.LinearMechanism(_linmodadd_cur_c, _linmodadd_cur_g, _linmodadd_cur_y, _linmodadd_cur_b, _cur_sec_list, _cur_x_list)
        else:
            _linmodadd_cur = None
    _cvode_object.re_init()    

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
_cvode_object.extra_scatter_gather(0, _advance2)
_cvode_object.extra_scatter_gather(1, _do_imports)


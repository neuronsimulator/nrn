import weakref
from neuron import h
from . import node, rxdsection, nodelist, morphology
import numpy
from .rxdException import RxDException

# all concentration ptrs and indices
_all_cptrs = []
_all_cindices = []
_c_ptr_vector = None
_c_ptr_vector_storage_nrn = None
_c_ptr_vector_storage = None
_last_c_ptr_length = None

def _donothing(): pass

def _purge_cptrs():
    """purges all cptr information"""
    global _all_cptrs, _all_cindices, _c_ptr_vector, _last_c_ptr_length
    global _c_ptr_vector_storage, _c_ptr_vector_storage_nrn
    _all_cptrs = []
    _all_cindices = []
    _c_ptr_vector = None
    _c_ptr_vector_storage_nrn = None
    _c_ptr_vector_storage = None
    _last_c_ptr_length = None

def _transfer_to_legacy():
    global  _c_ptr_vector, _c_ptr_vector_storage, _c_ptr_vector_storage_nrn
    global _last_c_ptr_length
    
    size = len(_all_cptrs)
    if _last_c_ptr_length != size:
        if size:
            _c_ptr_vector = h.PtrVector(size)
            _c_ptr_vector.ptr_update_callback(_donothing)
            for i, ptr in enumerate(_all_cptrs):
                _c_ptr_vector.pset(i, ptr)
            _c_ptr_vector_storage_nrn = h.Vector(size)
            _c_ptr_vector_storage = _c_ptr_vector_storage_nrn.as_numpy()
        else:
            _c_ptr_vector = None
        _last_c_ptr_length = size
    if size:
        _c_ptr_vector_storage[:] = node._get_states()[_all_cindices]
        _c_ptr_vector.scatter(_c_ptr_vector_storage_nrn)

class Section1D(rxdsection.RxDSection):
    def __init__(self, species, sec, diff, r):
        self._species = weakref.ref(species)
        self._diff = diff
        self._sec = sec
        self._concentration_ptrs = None
        self._offset = node._allocate(sec.nseg + 1)
        self._nseg = sec.nseg
        self._region = r
        # NOTE: you must do _init_diffusion_rates after assigning parents
    
    def _init_diffusion_rates(self):
        # call only after roots are set
        node._diffs[self._offset : self._offset + self.nseg] = self._diff
    
    def _update_node_data(self):
        volumes, surface_area, diffs = node._get_data()
        geo = self._region._geometry
        volumes[self._offset : self._offset + self.nseg] = geo.volumes1d(self)
        surface_area[self._offset : self._offset + self.nseg] = geo.surface_areas1d(self)
        self._neighbor_areas = geo.neighbor_areas1d(self)
        

    @property
    def indices(self):
        return list(range(self._offset, self._offset + self.nseg))

        
    def _setup_currents(self, indices, scales, ptrs, cur_map):
        from . import rxd
        if self.nrn_region is not None and self.species.name is not None and self.species.charge != 0:
            ion_curr = '_ref_i%s' % self.species.name
            indices.extend(self.indices)
            volumes, surface_area, diffs = node._get_data()
            # TODO: this implicitly assumes that o and i border the membrane
            # different signs depending on if an outward current decreases the region's concentration or increases it
            if self.nrn_region == 'i':
                sign = -1
            elif self.nrn_region == 'o':
                sign = 1
            else:
                raise RxDException('bad nrn_region for setting up currents (should never get here)')
            scales.extend(sign * surface_area[self.indices] * 10000. / (self.species.charge * rxd.FARADAY * volumes[self.indices]))
            for i in range(self.nseg):
                cur_map[self.species.name + self.nrn_region][self._sec((i + 0.5) / self.nseg)] = len(ptrs) + i
            ptrs.extend([self._sec((i + 0.5) / self.nseg).__getattribute__(ion_curr) for i in range(self.nseg)])

    @property
    def nodes(self):
        dx = self.L / self.nseg
        return nodelist.NodeList([node.Node1D(self, i, ((i + 0.5) * dx) / self.L) for i in range(self.nseg)])
            
    def _transfer_to_legacy(self):
        states = node._get_states()
        if self._concentration_ptrs is not None:
            for i, ptr in zip(range(self._offset, self._offset + self.nseg), self._concentration_ptrs):
                ptr[0] = states[i]
        
    def _register_cptrs(self):
        global _all_cptrs, _all_cindices
        if self.nrn_region is not None and self.species.name is not None:
            ion = '_ref_' + self.species.name + self.nrn_region
            nseg = self.nseg
            for i in range(nseg):
                x = (i + 0.5) / nseg
                _all_cptrs.append(self._sec(x).__getattribute__(ion))
                _all_cindices.append(self._offset + i)
            self._concentration_ptrs = _all_cptrs[-nseg :]
        else:
            self._concentration_ptrs = []

    def _setup_diffusion_matrix(self, g):
        _volumes, _surface_area, _diffs = node._get_data()
        offset = self._offset
        dx = self.L / self.nseg
        #print 'volumes:', _volumes
        #print 'areas:', self._neighbor_areas
        for i in range(self.nseg):
            io = i + offset
            if i > 0:
                il = io - 1
            else:
                parent, parenti = self._parent
                il = parent._offset + parenti
            # second order accuracy needs diffusion constants halfway
            # between nodes, which we approx by averaging
            # TODO: is this the best way to handle boundary nodes?
            if i > 0:
                d_l = (_diffs[io] + _diffs[io - 1]) / 2.
            else:
                d_l = _diffs[io]
            if i < self.nseg - 1:
                d_r = (_diffs[io] + _diffs[io + 1]) / 2.
            else:
                d_r = _diffs[io]
            rate_l = d_l * self._neighbor_areas[i] / (_volumes[io] * dx)
            if i == 0:
                # on left edge, only half distance
                # TODO: verify this is the right thing to do
                rate_l *= 2
            rate_r = d_r * self._neighbor_areas[i + 1] / (_volumes[io] * dx)
            if i == self.nseg - 1:
                # on right edge, only half distance
                # TODO: verify this is the right thing to do
                rate_r *= 2
            # TODO: does this try/except add overhead? remove
            try:
                g[io, io] += rate_r + rate_l
                g[io, io + 1] -= rate_r
                g[io, il] -= rate_l
            except IndexError:
                print('indexerror: g.shape = %r, io = %r, il = %r, len(node._states) = %r' % (g.shape, io, il, len(node._states)))
                raise
            # TODO: verify that these are correct
            if i == 0:
                # for 0 volume nodes, must conserve MASS not concentration
                scale = _volumes[io] / _volumes[il] if _volumes[il] else _volumes[io]
                g[il, il] += rate_l * scale
                g[il, io] -= rate_l * scale
            if i == self.nseg - 1:
                ir = io + 1
                # for 0 volume nodes, must conserve MASS not concentration
                # I suspect ir always points to a 0 volume node, but
                # good to be safe
                scale = _volumes[io] / _volumes[ir] if _volumes[ir] else _volumes[io]
                g[ir, ir] += rate_r * scale
                g[ir, io] -= rate_r * scale
            

    def _import_concentration(self, init):
        """imports concentration from NEURON; else 0s it if not in NEURON"""
        states = node._get_states()
        if self.nrn_region is not None and self.species.name is not None:
            for i, ptr in zip(range(self._offset, self._offset + self.nseg), self._concentration_ptrs):
                states[i] = ptr[0]

        elif init:
            # TODO: think about if this is the desired behavior
            for i in range(self.nseg):
                states[i + self._offset] = 0

    def _assign_parents(self, root_id, missing, root_children):
        # assign parents and root nodes
        parent_sec = morphology.parent(self._sec)
        if parent_sec is None or not self._species()._has_region_section(self._region, parent_sec):
            if parent_sec is None:
                pt = (self._region, None)
            else:
                pt = (self._region, parent_sec)
            if parent_sec is None or pt not in missing:
                missing[pt] = root_id
                root_id += 1
                root_children.append([])
            local_root = missing[pt]
            # TODO: which way do I want to do the first entry here?
            #self._parent = (self, local_root)
            self._parent = (self.species, local_root)
            root_children[local_root].append(self)
        else:
            # TODO: this is inefficient since checking the list twice for _region, parent_sec combo
            #       but doesn't matter much since only done at setup
            parent_section = self.species._region_section(self._region, parent_sec)
            self._parent = (parent_section, int(parent_sec.nseg * morphology.parent_loc(self._sec, parent_sec)))
        #print 'parent:', self._parent
        return root_id






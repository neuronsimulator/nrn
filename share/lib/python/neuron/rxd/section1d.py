import weakref
import node
from neuron import h
import numpy
import rxd
import rxdsection
import nodelist

# all concentration ptrs and indices
_all_cptrs = []
_all_cindices = []

def _purge_cptrs():
    """purges all cptr information"""
    global _all_cptrs, _all_cindices
    _all_cptrs = []
    _all_cindices = []

def _transfer_to_legacy():
    # TODO: this is ridiculous; move into C++?
    concentrations = numpy.array(node._get_states().to_python)[_all_cindices]
    for ptr, c in zip(_all_cptrs, concentrations): ptr[0] = c

def _parent(sec):
    """Return the parent of sec or None if sec is a root"""
    sref = h.SectionRef(sec=sec)
    return sref.trueparent().sec if sref.has_trueparent() else None

def _parent_loc(sec):
    """Return the position on the (true) parent where sec is connected
    
    Note that h.section_orientation(sec=sec) is which end of the section is
    connected.
    """
    # TODO: would I ever have a parent but not a trueparent (or vice versa)
    sref = h.SectionRef(sec=sec)
    if not sref.has_trueparent():
        return None
    trueparent = sref.trueparent().sec
    parent = sref.parent().sec
    while parent.name() != trueparent.name():
        sec, parent = parent, h.SectionRef(sec=sec).parent().sec
    return h.parent_connection(sec=sec)


class Section1D(rxdsection.RxDSection):
    def __init__(self, species, sec, diff, r):
        self._species = weakref.ref(species)
        self._diff = diff
        self._sec = sec
        self._concentration_ptrs = None
        self._offset = node._allocate(sec.nseg + 1)
        self._nseg = sec.nseg
        self._region = r
    
    def _update_node_data(self):
        volumes, surface_area, diffs = node._get_data()
        geo = self._region._geometry
        volumes[self._offset : self._offset + self.nseg] = geo.volumes1d(self)
        surface_area[self._offset : self._offset + self.nseg] = geo.surface_areas1d(self)
        # TODO: if diffs changed locally, this will overwrite them; fix
        diffs[self._offset : self._offset + self.nseg] = self._diff
        self._neighbor_areas = geo.neighbor_areas1d(self)
        

    @property
    def indices(self):
        return range(self._offset, self._offset + self.nseg)

        
    def _setup_currents(self, indices, scales, ptrs):
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
                raise Exception('bad nrn_region for setting up currents (should never get here)')
            scales.extend(sign * surface_area[self.indices] * 10000. / (self.species.charge * rxd.FARADAY * volumes[self.indices]))
            ptrs.extend([self._sec((i + 0.5) / self.nseg).__getattribute__(ion_curr) for i in xrange(self.nseg)])

    @property
    def nodes(self):
        dx = self.L / self.nseg
        return nodelist.NodeList([node.Node(self, i, ((i + 0.5) * dx) / self.L) for i in xrange(self.nseg)])
            
    def _transfer_to_legacy(self):
        states = node._get_states()
        for i, ptr in zip(xrange(self._offset, self._offset + self.nseg), self._concentration_ptrs):
            ptr[0] = states.x[i]
        
    def _register_cptrs(self):
        global _all_cptrs, _all_cindices
        if self.nrn_region is not None and self.species.name is not None:
            ion = '_ref_' + self.species.name + self.nrn_region
            nseg = self.nseg
            for i in xrange(nseg):
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
        for i in xrange(self.nseg):
            io = i + offset
            gi0 = g.getval(io, io)
            gir = g.getval(io, io + 1)
            if i > 0:
                il = io - 1
            else:
                parent, parenti = self._parent
                il = parent._offset + parenti
            gil = g.getval(io, il)
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
            g.setval(io, io, gi0 + rate_r + rate_l)
            g.setval(io, io + 1, gir - rate_r)
            g.setval(io, il, gil - rate_l)
            # TODO: verify that these are correct
            if i == 0:
                gil2 = g.getval(il, il)
                g.setval(il, il, gil2 + rate_l * _volumes[io])
                gilio = g.getval(il, io)
                g.setval(il, io, gilio - rate_l * _volumes[io])
            if i == self.nseg - 1:
                ir = io + 1
                gir2 = g.getval(ir, ir)
                girio = g.getval(ir, io)
                g.setval(ir, ir, gir2 + rate_r * _volumes[io])
                g.setval(ir, io, girio - rate_r * _volumes[io])
            

    def _import_concentration(self, init):
        """imports concentration from NEURON; else 0s it if not in NEURON"""
        states = node._get_states()
        if self.nrn_region is not None and self.species.name is not None:
            for i, ptr in zip(xrange(self._offset, self._offset + self.nseg), self._concentration_ptrs):
                states.x[i] = ptr[0]

        elif init:
            # TODO: think about if this is the desired behavior
            for i in xrange(self.nseg):
                states.x[i + self._offset] = 0

    def _assign_parents(self, root_id, missing, root_children):
        # assign parents and root nodes
        parent_sec = _parent(self._sec)
        if parent_sec is None or not self._species()._has_region_section(self._region, parent_sec):
            if parent_sec is None:
                pt = (self._region, None)
            else:
                pt = (self._region, parent_sec.name())
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
            parent_section = self.species._region_section(self._region, parent_sec)
            self._parent = (parent_section, int(parent_section.nseg * _parent_loc(self._sec)))
        return root_id






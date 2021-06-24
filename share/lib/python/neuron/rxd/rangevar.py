import numpy
from neuron import h

_h_vector = h.Vector
_numpy_zeros = numpy.zeros
_h_ptrvector = h.PtrVector


def _donothing():
    pass


class RangeVar:
    def __init__(self, name):
        self._name = name
        self._ptr_vector = None

    def _init_ptr_vectors(self, nodes):
        # TODO: make sure this gets changed everytime there is a structurechange event
        #       or everytime the nodes change
        ptrs = []
        locs = []
        ptrs_append = ptrs.append
        locs_append = locs.append
        name = self._name
        for node in nodes:
            seg = node.segment
            ptrs_append(getattr(seg, "_ref_%s" % name))
            # TODO: is this the right index? or do I need to change things to
            #       account for the zero-volume nodes?
            locs_append(node._index)
        self._locs = numpy.array(locs)
        pv = _h_ptrvector(len(ptrs))
        pv.ptr_update_callback(_donothing)
        pv_pset = pv.pset
        for i, ptr in enumerate(ptrs):
            pv_pset(i, ptr)
        self._pv = pv

    def _rangevar_vec(self):
        pv = self._pv
        result = _numpy_zeros(pv.size())
        vec = _h_vector(pv.size())
        pv.gather(vec)
        vec.to_python(result)
        return result

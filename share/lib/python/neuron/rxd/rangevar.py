import numpy
from neuron import h

class RangeVar:
    def __init__(self, name):
        self._name = name
        self._ptr_vector = None
        
    def _init_ptr_vectors(self, nodes):
        # TODO: make sure this gets changed everytime there is a structurechange event
        #       or everytime the nodes change
        ptrs = []
        locs = []
        name = self._name
        for node in nodes:
            seg = node.segment
            ptrs.append(seg.__getattribute__('_ref_%s' % name))
            # TODO: is this the right index? or do I need to change things to
            #       account for the zero-volume nodes?
            locs.append(node._index)
        self._locs = numpy.array(locs)
        pv = h.PtrVector(len(ptrs))
        for i, ptr in enumerate(ptrs):
            pv.pset(i, ptr)
        self._pv = pv
        
    def _rangevar_vec(self):
        pv = self._pv
        result = numpy.zeros(pv.size())
        vec = h.Vector(pv.size())
        pv.gather(vec)
        vec.to_python(result)
        return result

                
from neuronmusic cimport *

cdef class EventOutputPort:
    cdef cxx_EventOutputPort* cxx # hold a C++ instance which we're wrapping
    
    def __cinit__ (self):
        self.cxx = NULL

    def __dealloc__ (self):
        del_EventOutputPort (self.cxx)

    def gid2index (self, int gid, int gi):
        if self.cxx != NULL:
            self.cxx.gid2index (gid, gi)

cdef wrapEventOutputPort (cxx_EventOutputPort* port):
    cdef EventOutputPort port_ = EventOutputPort ()
    port_.cxx = port
    return port_


cdef class EventInputPort:
    cdef cxx_EventInputPort* cxx # hold a C++ instance which we're wrapping
    
    def __cinit__ (self):
        self.cxx = NULL

    def __dealloc__ (self):
        del_EventInputPort (self.cxx)

    def index2target (self, int gi, target):
        if self.cxx != NULL:
            return self.cxx.index2target (gi, target)
        else:
            return False

cdef wrapEventInputPort (cxx_EventInputPort* port):
    cdef EventInputPort port_ = EventInputPort ()
    port_.cxx = port
    return port_


def publishEventOutput (identifier):
    return wrapEventOutputPort (cxx_publishEventOutput (identifier.encode ('utf-8')))


def publishEventInput (identifier):
    return wrapEventInputPort (cxx_publishEventInput (identifier.encode ('utf-8')))


# Local Variables:
# mode: python
# End:

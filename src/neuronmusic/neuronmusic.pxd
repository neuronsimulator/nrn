cdef extern from "music.hh":
    ctypedef struct cxx_Setup "MUSIC::Setup"

cdef extern from "nrnmusic.h":
    ctypedef struct cxx_EventOutputPort "NRNMUSIC::EventOutputPort":
       void gid2index (int gid, int gi)

    cxx_EventOutputPort* new_EventOutputPort "new NRNMUSIC::EventOutputPort" (cxx_Setup* s, char* id)
    void del_EventOutputPort "delete" (cxx_EventOutputPort* obj)

    ctypedef struct cxx_EventInputPort "NRNMUSIC::EventInputPort":
       object index2target (int gi, object target)

    cxx_EventInputPort* new_EventInputPort "new NRNMUSIC::EventInputPort" (cxx_Setup* s, char* id)
    void del_EventInputPort "delete" (cxx_EventInputPort* obj)

    cxx_EventOutputPort* cxx_publishEventOutput "NRNMUSIC::publishEventOutput" (char* identifier)

    cxx_EventInputPort* cxx_publishEventInput "NRNMUSIC::publishEventInput" (char* identifier)


# Local Variables:
# mode: python
# End:

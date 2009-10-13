#ifndef nrnmusic_h
#define nrnmusic_h

#include <music.hh>
struct PyObject;
class PreSyn;

// Interface which nrnmusic.so module (which mdj writes) will use

namespace NRNMUSIC {

  class EventOutputPort : public MUSIC::EventOutputPort {
  public:
    // Connect a gid to a MUSIC global index in this port
    void gid2index (int gid, int gi);
  };

  class EventInputPort : public MUSIC::EventInputPort {
  public:
    // Connect a MUSIC global index to a gid in this port
    void index2netcon (int gi, PyObject* netcon);
  };

  EventOutputPort* publishEventOutput (string identifier);

  EventInputPort* publishEventInput (string identifier);

}

#endif

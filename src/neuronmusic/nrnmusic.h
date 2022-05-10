#include <string>

#if defined(NO_PYTHON_H)
typedef struct _object PyObject;
#else
#include <nrnwrap_Python.h>
#endif
#include <music.hh>
#include <map>

class PreSyn;

// Interface which nrnmusic.so module (which mdj writes) will use

namespace NRNMUSIC {

class EventOutputPort: public MUSIC::EventOutputPort {
  public:
    EventOutputPort(MUSIC::Setup* s, std::string id)
        : MUSIC::Port(s, id)
        , MUSIC::EventOutputPort(s, id) {}

    // Connect a gid to a MUSIC global index in this port
    void gid2index(int gid, int gi);

    std::map<int, PreSyn*> gi_table;
};

class EventInputPort: public MUSIC::EventInputPort {
  public:
    EventInputPort(MUSIC::Setup* s, std::string id);

    // Connect a MUSIC global index to a gid in this port
    PyObject* index2target(int gi, PyObject* target);

    std::map<int, PreSyn*> gi_table;
};

EventOutputPort* publishEventOutput(std::string identifier);

EventInputPort* publishEventInput(std::string identifier);

}  // namespace NRNMUSIC

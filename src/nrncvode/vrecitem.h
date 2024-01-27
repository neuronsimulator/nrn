#ifndef vrecitem_h
#define vrecitem_h

#include <InterViews/observe.h>
#include <netcon.h>
#include <ivocvect.h>

class PlayRecord;
class PlayRecordSave;
class VecRecordDiscreteSave;
class VecPlayStepSave;
class VecPlayContinuousSave;
class StmtInfo;
struct NrnThread;
struct Section;

// SaveState subtypes for PlayRecordType and trajectory return type
#define VecRecordDiscreteType 1
#define VecRecordDtType       2
#define VecPlayStepType       3
#define VecPlayContinuousType 4
#define TvecRecordType        5
#define YvecRecordType        6
#define GLineRecordType       7
#define GVectorRecordType     8

// used by PlayRecord subclasses that utilize discrete events
class PlayRecordEvent: public DiscreteEvent {
  public:
    PlayRecordEvent();
    virtual ~PlayRecordEvent();
    virtual void deliver(double, NetCvode*, NrnThread*);
    virtual void pr(const char*, double t, NetCvode*);
    virtual void frecord_init(TQItem* q);
    virtual NrnThread* thread();
    virtual int type() {
        return PlayRecordEventType;
    }
    PlayRecord* plr_;
    static unsigned long playrecord_send_;
    static unsigned long playrecord_deliver_;

    virtual DiscreteEvent* savestate_save();
    virtual void savestate_restore(double deliverytime, NetCvode*);
    virtual void savestate_write(FILE*);
    static DiscreteEvent* savestate_read(FILE*);
};

// common interface for Play and Record for all integration methods.
class PlayRecord: public Observer {
  public:
    PlayRecord(neuron::container::data_handle<double> pd, Object* ppobj = nullptr);
    virtual ~PlayRecord();
    virtual void install(Cvode* cv) {
        cvode_ = cv;
    }                              // cvode play or record list?
    virtual void play_init() {}    // called near beginning of finitialize
    virtual void record_init() {}  // called near end of finitialize and frecord_init()
    virtual void continuous(double t) {
    }  // play - every f(y, t) or res(y', y, t); record - advance_tn and initialize flag
    virtual void deliver(double t, NetCvode*) {}  // at associated DiscreteEvent
    virtual PlayRecordEvent* event() {
        return nullptr;
    }
    virtual void pr();  // print identifying info
    virtual int type() {
        return 0;
    }

    // install normally calls one of these. Cvode may be nullptr.
    void play_add(Cvode*);
    void record_add(Cvode*);

    // administration
    virtual void disconnect(Observable*);
    virtual void update(Observable* o) {
        disconnect(o);
    }
    virtual bool uses(void*) {
        return false;
    }
    virtual void frecord_init(TQItem*) {}
    // for example, subclasses use things that may not go out of existence but wish us
    // to remove ourselves from the PlayRecord system. e.g Vector.play_remove().

    virtual PlayRecordSave* savestate_save();
    static PlayRecordSave* savestate_read(FILE*);

    // pd_ can refer to a voltage, and those are stored in a modern container,
    // so we need to use data_handle
    neuron::container::data_handle<double> pd_;
    Object* ppobj_;
    Cvode* cvode_;
    int ith_;  // The thread index
};

class PlayRecordSave {
  public:
    PlayRecordSave(PlayRecord*);
    virtual ~PlayRecordSave();
    virtual void savestate_restore(){};
    virtual void savestate_write(FILE*) {}
    virtual void savestate_read(FILE*) {}
    void check();

    PlayRecord* pr_;
    int prl_index_;
};

class TvecRecord: public PlayRecord {
  public:
    TvecRecord(Section*, IvocVect* tvec, Object* ppobj = nullptr);
    virtual ~TvecRecord();
    virtual void install(Cvode*);
    virtual void record_init();
    virtual void continuous(double t);
    virtual int type() {
        return TvecRecordType;
    }

    virtual void disconnect(Observable*);
    virtual bool uses(void* v) {
        return (void*) t_ == v;
    }

    IvocVect* t_;
    Section* sec_;
};

class YvecRecord: public PlayRecord {
  public:
    YvecRecord(neuron::container::data_handle<double>, IvocVect* y, Object* ppobj = nullptr);
    virtual ~YvecRecord();
    virtual void install(Cvode*);
    virtual void record_init();
    virtual void continuous(double t);
    virtual int type() {
        return YvecRecordType;
    }

    virtual void disconnect(Observable*);
    virtual bool uses(void* v) {
        return (void*) y_ == v;
    }

    IvocVect* y_;
};

class VecRecordDiscrete: public PlayRecord {
  public:
    VecRecordDiscrete(neuron::container::data_handle<double>,
                      IvocVect* y,
                      IvocVect* t,
                      Object* ppobj = nullptr);
    virtual ~VecRecordDiscrete();
    virtual void install(Cvode*);
    virtual void record_init();
    virtual PlayRecordEvent* event() {
        return e_;
    }
    virtual void deliver(double t, NetCvode*);

    virtual void disconnect(Observable*);
    virtual bool uses(void* v) {
        return (void*) y_ == v || (void*) t_ == v;
    }

    virtual void frecord_init(TQItem*);

    virtual int type() {
        return VecRecordDiscreteType;
    }
    virtual PlayRecordSave* savestate_save();

    IvocVect* y_;
    IvocVect* t_;
    PlayRecordEvent* e_;
};

class VecRecordDiscreteSave: public PlayRecordSave {
  public:
    VecRecordDiscreteSave(PlayRecord*);
    virtual ~VecRecordDiscreteSave();
    virtual void savestate_restore();
    virtual void savestate_write(FILE*);
    virtual void savestate_read(FILE*);
    int cursize_;
};

class VecRecordDt: public PlayRecord {
  public:
    VecRecordDt(neuron::container::data_handle<double>,
                IvocVect* y,
                double dt,
                Object* ppobj = nullptr);
    virtual ~VecRecordDt();
    virtual void install(Cvode*);
    virtual void record_init();
    virtual void deliver(double t, NetCvode*);
    virtual PlayRecordEvent* event() {
        return e_;
    }

    virtual void disconnect(Observable*);
    virtual bool uses(void* v) {
        return (void*) y_ == v;
    }

    virtual void frecord_init(TQItem*);
    virtual int type() {
        return VecRecordDtType;
    }
    virtual PlayRecordSave* savestate_save();

    IvocVect* y_;
    double dt_;
    PlayRecordEvent* e_;
};

class VecRecordDtSave: public PlayRecordSave {
  public:
    VecRecordDtSave(PlayRecord*);
    virtual ~VecRecordDtSave();
    virtual void savestate_restore();
};

class VecPlayStep: public PlayRecord {
  public:
    VecPlayStep(neuron::container::data_handle<double>,
                IvocVect* y,
                IvocVect* t,
                double dt,
                Object* ppobj = nullptr);
    VecPlayStep(const char* s, IvocVect* y, IvocVect* t, double dt, Object* ppobj = nullptr);
    void init(IvocVect* y, IvocVect* t, double dt);
    virtual ~VecPlayStep();
    virtual void install(Cvode*);
    virtual void play_init();
    virtual void deliver(double t, NetCvode*);
    virtual PlayRecordEvent* event() {
        return e_;
    }
    virtual void pr();

    virtual void disconnect(Observable*);
    virtual bool uses(void* v) {
        return (void*) y_ == v || (void*) t_ == v;
    }
    virtual int type() {
        return VecPlayStepType;
    }
    virtual PlayRecordSave* savestate_save();

    IvocVect* y_;
    IvocVect* t_;
    double dt_;
    int current_index_;

    PlayRecordEvent* e_;
    StmtInfo* si_;
};
class VecPlayStepSave: public PlayRecordSave {
  public:
    VecPlayStepSave(PlayRecord*);
    virtual ~VecPlayStepSave();
    virtual void savestate_restore();
    virtual void savestate_write(FILE*);
    virtual void savestate_read(FILE*);
    int curindex_;
};

class VecPlayContinuous: public PlayRecord {
  public:
    VecPlayContinuous(neuron::container::data_handle<double> pd,
                      IvocVect* y,
                      IvocVect* t,
                      IvocVect* discon,
                      Object* ppobj = nullptr);
    VecPlayContinuous(const char* s,
                      IvocVect* y,
                      IvocVect* t,
                      IvocVect* discon,
                      Object* ppobj = nullptr);
    virtual ~VecPlayContinuous();
    void init(IvocVect* y, IvocVect* t, IvocVect* tdiscon);
    virtual void install(Cvode*);
    virtual void play_init();
    virtual void deliver(double t, NetCvode*);
    virtual PlayRecordEvent* event() {
        return e_;
    }
    virtual void pr();

    void continuous(double tt);
    double interpolate(double tt);
    double interp(double th, double x0, double x1) {
        return x0 + (x1 - x0) * th;
    }
    void search(double tt);

    virtual void disconnect(Observable*);
    virtual bool uses(void* v) {
        return (void*) y_ == v || (void*) t_ == v || (void*) discon_indices_ == v;
    }
    virtual int type() {
        return VecPlayContinuousType;
    }
    virtual PlayRecordSave* savestate_save();

    IvocVect* y_;
    IvocVect* t_;
    IvocVect* discon_indices_;
    int last_index_;
    int discon_index_;
    int ubound_index_;

    PlayRecordEvent* e_;
    StmtInfo* si_;
};
class VecPlayContinuousSave: public PlayRecordSave {
  public:
    VecPlayContinuousSave(PlayRecord*);
    virtual ~VecPlayContinuousSave();
    virtual void savestate_restore();
    virtual void savestate_write(FILE*);
    virtual void savestate_read(FILE*);
    int last_index_;
    int discon_index_;
    int ubound_index_;
};

#endif

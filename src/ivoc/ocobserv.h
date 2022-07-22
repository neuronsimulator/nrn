#ifndef ocobserve_h
#define ocobserve_h

#include "oc_ansi.h"

#include <InterViews/observe.h>

struct Object;
struct cTemplate;

// For an Observer watching a hoc Object
// when the last ref disappears, disconnect is called on the Observer
// Some objects may be written so that update gets called  on the Observer

class ObjObservable: public Observable {
  public:
    static void Attach(Object*, Observer*);
    static void Detach(Object*, Observer*);
    virtual ~ObjObservable();

    Object* object() {
        return ob_;
    }

  private:
    ObjObservable(Object*);

  private:
    Object* ob_;
};

// For an Observer watching a cTemplate
class ClassObservable: public Observable {
  public:
    // only the first two guarantee an update on the Observer
    enum { Delete, Create, Changed };

    static void Attach(cTemplate*, Observer*);
    static void Detach(cTemplate*, Observer*);
    virtual ~ClassObservable();

    cTemplate* ctemplate() {
        return ct_;
    }
    Object* object() {
        return ob_;
    }
    int message() {
        return message_;
    }

  private:
    friend void hoc_template_notify(Object*, int);
    ClassObservable(cTemplate*);
    void attach(Observer*);
    void detach(Observer*);

  private:
    cTemplate* ct_;
    Object* ob_;
    int message_;
    int cnt_;
};

#endif

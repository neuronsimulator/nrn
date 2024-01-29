#include <../../nrnconf.h>
#include "ocobserv.h"
#include "oc2iv.h"

void hoc_obj_disconnect(Object* ob) {
    delete ((ObjObservable*) ob->observers);
    ob->observers = NULL;
}

void hoc_obj_notify(Object* ob) {
    if (ob->observers) {
        ((ObjObservable*) ob->observers)->notify();
    }
}

void ObjObservable::Attach(Object* ob, Observer* view) {
    if (!ob->observers) {
        ob->observers = (void*) (new ObjObservable(ob));
    }
    ((ObjObservable*) ob->observers)->attach(view);
}

void ObjObservable::Detach(Object* ob, Observer* view) {
    if (!ob->observers) {
        return;
    }
    ((ObjObservable*) ob->observers)->detach(view);
}

ObjObservable::ObjObservable(Object* ob) {
    ob_ = ob;
}

ObjObservable::~ObjObservable() {}


void hoc_template_notify(Object* ob, int message) {
    ClassObservable* co = (ClassObservable*) ob->ctemplate->observers;
    if (co) {
        co->ob_ = ob;
        co->message_ = message;
        co->notify();
    }
}

void ClassObservable::Attach(cTemplate* ct, Observer* view) {
    if (!ct->observers) {
        ct->observers = (void*) (new ClassObservable(ct));
    }
    ClassObservable* co = (ClassObservable*) ct->observers;
    co->attach(view);
}

void ClassObservable::Detach(cTemplate* ct, Observer* view) {
    if (!ct->observers) {
        return;
    }
    ClassObservable* co = (ClassObservable*) ct->observers;
    co->detach(view);
    if (co->cnt_ <= 0) {
        delete co;
    }
}

ClassObservable::ClassObservable(cTemplate* ct) {
    ct_ = ct;
    ob_ = NULL;
    message_ = 0;
    cnt_ = 0;
}

ClassObservable::~ClassObservable() {
    ct_->observers = NULL;
}

void ClassObservable::attach(Observer* o) {
    Observable::attach(o);
    ++cnt_;
}

void ClassObservable::detach(Observer* o) {
    Observable::detach(o);
    --cnt_;
}

#include "classreg.h"
#include "nrnoc2iv.h"

#include "tqueue.hpp"

static double insert(void* v) {
    TQueue* q = (TQueue*) v;
    q->insert(*getarg(1), (void*) 1);
    return 1.;
}
static double print(void* v) {
    TQueue* q = (TQueue*) v;
    q->print();
    return 1.;
}

static double least(void* v) {
    TQueue* q = (TQueue*) v;
    TQItem* i = q->least();
    double x = -1e9;
    if (i) {
        x = i->t_;
    }
    return x;
}
static double rmleast(void* v) {
    TQueue* q = (TQueue*) v;
    TQItem* i = q->least();
    double x = -1e9;
    if (i) {
        x = i->t_;
        q->remove(i);
    }
    return x;
}

static double mvleast(void* v) {
    TQueue* q = (TQueue*) v;
    q->move_least(*getarg(1));
    return 1.;
}

static double remove(void* v) {
    TQueue* q = (TQueue*) v;
    q->remove(q->find(*getarg(1)));
    return 1.;
}

static double find(void* v) {
    TQueue* q = (TQueue*) v;
    TQItem* i = q->find(*getarg(1));
    double x = -1e9;
    if (i) {
        x = i->t_;
        q->remove(i);
    }
    return x;
}
static double stats(void* v) {
    TQueue* q = (TQueue*) v;
    q->statistics();
    return 1.;
}

static Member_func members[] = {{"insrt", insert},
                                {"least", least},
                                {"move_least", mvleast},
                                {"remove_least", rmleast},
                                {"remove", remove},
                                {"find", find},
                                {"stats", stats},
                                {"printf", print},
                                {nullptr, nullptr}};

static void* cons(Object*) {
    assert(0);
    TQueue* q = new TQueue(0);
    return (void*) q;
}

static void destruct(void* v) {
    TQueue* q = (TQueue*) v;
    delete q;
}

void TQueue_reg() {
    class2oc("TQueue", cons, destruct, members, nullptr, nullptr, nullptr);
}

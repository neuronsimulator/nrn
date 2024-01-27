#include <../../nrnconf.h>
#include "tqueue.h"
#include "pool.h"

#include "classreg.h"
#include "nrnoc2iv.h"

#define PROFILE 0
#include "profile.h"

#define DOCHECK 0

#if COLLECT_TQueue_STATISTICS
#define STAT(arg) ++arg;
#else
#define STAT(arg) /**/
#endif

static const char* errmess_;

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
                                {0, 0}};

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
    class2oc("TQueue", cons, destruct, members, NULL, NULL, NULL);
}

//----------------

#if BBTQ == 0
#include <bbtqueue.cpp>
#elif BBTQ == 2
#include <sptqueue.cpp>
#elif BBTQ == 4
#include <spt2queue.cpp>
#elif BBTQ == 5
#include <sptbinq.cpp>
#endif

SelfQueue::SelfQueue(TQItemPool* tp, int mkmut) {
    MUTCONSTRUCT(mkmut)
    tpool_ = tp;
    head_ = nullptr;
}
SelfQueue::~SelfQueue() {
    remove_all();
    MUTDESTRUCT
}
TQItem* SelfQueue::insert(void* d) {
    MUTLOCK
    TQItem* q = tpool_->alloc();
    q->left_ = nullptr;
    q->right_ = head_;
    if (head_) {
        head_->left_ = q;
    }
    head_ = q;
    q->data_ = d;
    MUTUNLOCK
    return q;
}
void* SelfQueue::remove(TQItem* q) {
    MUTLOCK
    if (q->left_) {
        q->left_->right_ = q->right_;
    }
    if (q->right_) {
        q->right_->left_ = q->left_;
    }
    if (q == head_) {
        head_ = q->right_;
    }
    tpool_->hpfree(q);
    MUTUNLOCK
    return q->data_;
}
void SelfQueue::remove_all() {
    MUTLOCK
    for (TQItem* q = first(); q; q = next(q)) {
        tpool_->hpfree(q);
    }
    head_ = nullptr;
    MUTUNLOCK
}

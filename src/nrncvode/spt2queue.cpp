// splay tree + 2nd splay tree for event-sets or priority queues
// this starts from the sptqueue.cpp file and adds a 2ne splay tree

/* Derived from David Brower's c translation of pascal code by
Douglas Jones.
*/
/* The original c code is included from this file but note that instead
of struct _spblk, we are really using TQItem
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define leftlink  left_
#define rightlink right_
#define uplink    parent_
#define cnt       cnt_
#define key       t_
#include "sptree.h"

#if 0
#define sp1enq(i)          \
    {                      \
        i->cnt_ = 0;       \
        spenq(i, sptree_); \
    }
#define sp2enq(i)           \
    {                       \
        i->cnt_ = -1;       \
        spenq(i, sptree2_); \
    }
#else
#define sp1enq(i)          \
    {                      \
        i->cnt_ = 0;       \
        ++nenq1;           \
        spenq(i, sptree_); \
    }
#define sp2enq(i)           \
    {                       \
        i->cnt_ = -1;       \
        ++nenq2;            \
        spenq(i, sptree2_); \
    }
#endif

TQItem::TQItem() {
    left_ = 0;
    right_ = 0;
    parent_ = 0;
}

TQItem::~TQItem() {}

static void deleteitem(TQItem* i) {  // only one, semantics changed
    assert(i);
    tpool_->hpfree(i);
}

bool TQItem::check() {
#if DOCHECK
#endif
    return true;
}

static void prnt(const TQItem* b, int level) {
    int i;
    for (i = 0; i < level; ++i) {
        Printf("    ");
    }
    Printf("%g %c %d Q=%p D=%p\n", b->t_, b->data_ ? 'x' : 'o', b->cnt_, b, b->data_);
}

static void chk(TQItem* b, int level) {
    if (!b->check()) {
        hoc_execerror("chk failed", errmess_);
    }
}

TQueue::TQueue() {
    if (!tpool_) {
        tpool_ = new TQItemPool(1000);
    }
    sptree_ = new SPTREE;
    spinit(sptree_);
    sptree2_ = new SPTREE;
    spinit(sptree2_);
    least_ = 0;

#if COLLECT_TQueue_STATISTICS
    nmove = ninsert = nrem = nleast = nbal = ncmplxrem = 0;
    nfastmove = ncompare = nleastsrch = nfind = nfindsrch = 0;
    nenq1 = nenq2 = 0;
#endif
}

TQueue::~TQueue() {
    TQItem* q;
    if (least_) {
        deleteitem(least_);
    }
    while ((q = spdeq(&sptree_->root)) != nullptr) {
        deleteitem(q);
    }
    delete sptree_;
    while ((q = spdeq(&sptree2_->root)) != nullptr) {
        deleteitem(q);
    }
    delete sptree2_;
}

void TQueue::print() {
    if (least_) {
        prnt(least_, 0);
    }
    spscan(prnt, static_cast<TQItem*>(nullptr), sptree_);
    spscan(prnt, static_cast<TQItem*>(nullptr), sptree2_);
}

void TQueue::forall_callback(void (*f)(const TQItem*, int)) {
    if (least_) {
        f(least_, 0);
    }
    spscan(f, nullptr, sptree_);
    spscan(f, nullptr, sptree2_);
}

void TQueue::check(const char* mes) {}

void TQueue::move_least(double tnew) {
    TQItem* b = least();
    if (b) {
        b->t_ = tnew;
        TQItem* nl = sphead(sptree_);
        if (nl) {
            if (tnew <= nl->t_ && tnew <= q2least_t()) {
                ;
            } else if (nl->t_ <= q2least_t()) {
                least_ = spdeq(&sptree_->root);
                sp1enq(b);
            } else {
                least_ = spdeq(&sptree2_->root);
                sp1enq(b);
            }
        } else if (tnew > q2least_t()) {
            least_ = spdeq(&sptree2_->root);
            sp1enq(b);
        }
    }
}

void TQueue::move(TQItem* i, double tnew) {
    STAT(nmove)
    if (i == least_) {
        move_least(tnew);
    } else if (tnew < least_->t_) {
        spdelete(i, sptree_);
        i->t_ = tnew;
        sp1enq(least_);
        least_ = i;
    } else {
        spdelete(i, sptree_);
        i->t_ = tnew;
        sp1enq(i);
    }
}

void TQueue::statistics() {
#if COLLECT_TQueue_STATISTICS
    Printf("insertions=%lu  moves=%lu removals=%lu calls to least=%lu\n",
           ninsert,
           nmove,
           nrem,
           nleast);
    Printf("calls to find=%lu\n", nfind);
    Printf("comparisons=%lu\n", sptree_->enqcmps);
    Printf("neqn1=%lu nenq2=%lu\n", nenq1, nenq2);
#else
    Printf("Turn on COLLECT_TQueue_STATISTICS_ in tqueue.h\n");
#endif
}

void TQueue::spike_stat(double* d) {
#if COLLECT_TQueue_STATISTICS
    d[0] = ninsert;
    d[1] = nmove;
    d[2] = nrem;
    d[3] = nenq1;
    d[4] = nenq2;
#endif
}

TQItem* TQueue::insert(double t, void* d) {
    STAT(ninsert);
    TQItem* i = tpool_->alloc();
    i->data_ = d;
    i->t_ = t;
    i->cnt_ = 0;
    if (t < least_t()) {
        if (least()) {
            sp1enq(least());
        }
        least_ = i;
    } else {
        sp1enq(i);
    }
    return i;
}

TQItem* TQueue::insert_fifo(double t, void* d) {
    STAT(ninsert);
    TQItem* i = tpool_->alloc();
    i->data_ = d;
    i->t_ = t;
    i->cnt_ = 0;
    if (t < least_t()) {
        if (least()) {
            sp1enq(least());
        }
        least_ = i;
    } else {
        sp2enq(i);
    }
    return i;
}

void TQueue::remove(TQItem* q) {
    STAT(nrem);
    if (q) {
        if (q == least_) {
            if (sptree_->root) {
                TQItem* nl = sphead(sptree_);
                if (nl->t_ <= q2least_t()) {
                    least_ = spdeq(&sptree_->root);
                } else {
                    least_ = spdeq(&sptree2_->root);
                }
            } else if (sptree2_->root) {
                least_ = spdeq(&sptree2_->root);
            } else {
                least_ = nullptr;
            }
        } else {
            if (q->cnt_ == -1) {
                spdelete(q, sptree2_);
            } else {
                spdelete(q, sptree_);
            }
        }
        tpool_->hpfree(q);
    }
}

TQItem* TQueue::find(double t) {
    // search only in the  splay tree. if this is a bug then fix it.
    STAT(nfind)
    if (t == least_t()) {
        return least();
    }
    TQItem* q;
    q = splookup(t, sptree_);
    return (q);
}

double TQueue::q2least_t() {
    if (sptree2_->root) {
        return sphead(sptree2_)->t_;
    }
    return 1e50;  // must be larger than any possible t
}

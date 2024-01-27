// splay tree for event-sets or priority queues
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
#include <sptree.h>

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
    least_ = 0;

#if COLLECT_TQueue_STATISTICS
    nmove = ninsert = nrem = nleast = nbal = ncmplxrem = 0;
    nfastmove = ncompare = nleastsrch = nfind = nfindsrch = 0;
#endif
}

TQueue::~TQueue() {
    TQItem* q;
    while ((q = spdeq(&sptree_->root)) != nullptr) {
        deleteitem(q);
    }
    delete sptree_;
}

void TQueue::print() {
    if (least_) {
        prnt(least_, 0);
    }
    spscan(prnt, nullptr, sptree_);
}

void TQueue::forall_callback(void (*f)(const TQItem*, int)) {
    if (least_) {
        f(least_, 0);
    }
    spscan(f, nullptr, sptree_);
}

void TQueue::check(const char* mes) {}

TQItem* TQueue::second_least(double t) {
    assert(least_);
    TQItem* b = sphead(sptree_);
    if (b && b->t_ == t) {
        return b;
    }
    return 0;
}

void TQueue::move_least(double tnew) {
    TQItem* b = least();
    if (b) {
        b->t_ = tnew;
        TQItem* nl = sphead(sptree_);
        if (nl) {
            if (tnew > nl->t_) {
                least_ = spdeq(&sptree_->root);
                spenq(b, sptree_);
            }
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
        spenq(least_, sptree_);
        least_ = i;
    } else {
        spdelete(i, sptree_);
        i->t_ = tnew;
        spenq(i, sptree_);
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
#else
    Printf("Turn on COLLECT_TQueue_STATISTICS_ in tqueue.h\n");
#endif
}

void TQueue::spike_stat(double* d) {
#if COLLECT_TQueue_STATISTICS
    d[0] = ninsert;
    d[1] = nmove;
    d[2] = nrem;
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
            spenq(least(), sptree_);
        }
        least_ = i;
    } else {
        spenq(i, sptree_);
    }
    return i;
}

void TQueue::remove(TQItem* q) {
    STAT(nrem);
    if (q) {
        if (q == least_) {
            if (sptree_->root) {
                least_ = spdeq(&sptree_->root);
            } else {
                least_ = nullptr;
            }
        } else {
            spdelete(q, sptree_);
        }
        tpool_->hpfree(q);
    }
}

TQItem* TQueue::find(double t) {
    STAT(nfind)
    if (t == least_t()) {
        return least();
    }
    TQItem* q;
    q = splookup(t, sptree_);
    return (q);
}

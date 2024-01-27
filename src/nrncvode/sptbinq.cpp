// splay tree + bin queue limited to fixed step method
// for event-sets or priority queues
// this starts from the sptqueue.cpp file and adds a bin queue

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
#include <section.h>

#define leftlink  left_
#define rightlink right_
#define uplink    parent_
#define cnt       cnt_
#define key       t_
#include <sptree.h>

// extern double dt;
#define nt_dt nrn_threads->_dt

void (*nrn_binq_enqueue_error_handler)(double, TQItem*);

TQItem::TQItem() {
    left_ = 0;
    right_ = 0;
    parent_ = 0;
}

TQItem::~TQItem() {}

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

TQueue::TQueue(TQItemPool* tp, int mkmut) {
    MUTCONSTRUCT(mkmut)
    tpool_ = tp;
    nshift_ = 0;
    sptree_ = new SPTREE<TQItem>;
    spinit(sptree_);
    binq_ = new BinQ;
    least_ = 0;

#if COLLECT_TQueue_STATISTICS
    nmove = ninsert = nrem = nleast = nbal = ncmplxrem = 0;
    nfastmove = ncompare = nleastsrch = nfind = nfindsrch = 0;
#endif
}

TQueue::~TQueue() {
    TQItem *q, *q2;
    while ((q = spdeq(&sptree_->root)) != nullptr) {
        deleteitem(q);
    }
    delete sptree_;
    for (q = binq_->first(); q; q = q2) {
        q2 = binq_->next(q);
        remove(q);
    }
    delete binq_;
    MUTDESTRUCT
}

void TQueue::deleteitem(TQItem* i) {
    tpool_->hpfree(i);
}

void TQueue::print() {
    MUTLOCK
    if (least_) {
        prnt(least_, 0);
    }
    spscan(prnt, static_cast<TQItem*>(nullptr), sptree_);
    for (TQItem* q = binq_->first(); q; q = binq_->next(q)) {
        prnt(q, 0);
    }
    MUTUNLOCK
}

void TQueue::forall_callback(void (*f)(const TQItem*, int)) {
    MUTLOCK
    if (least_) {
        f(least_, 0);
    }
    spscan(f, static_cast<TQItem*>(nullptr), sptree_);
    for (TQItem* q = binq_->first(); q; q = binq_->next(q)) {
        f(q, 0);
    }
    MUTUNLOCK
}

void TQueue::check(const char* mes) {}

// for Parallel Global Variable Timestep method.
// Assume not using bin queue.
TQItem* TQueue::second_least(double t) {
    assert(least_);
    TQItem* b = sphead(sptree_);
    if (b && b->t_ == t) {
        return b;
    }
    return 0;
}

void TQueue::move_least(double tnew) {
    MUTLOCK
    move_least_nolock(tnew);
    MUTUNLOCK
}

void TQueue::move_least_nolock(double tnew) {
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
    MUTLOCK
    STAT(nmove)
    if (i == least_) {
        move_least_nolock(tnew);
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
    MUTUNLOCK
}

void TQueue::statistics() {
#if COLLECT_TQueue_STATISTICS
    Printf("insertions=%lu  moves=%lu removals=%lu calls to least=%lu\n",
           ninsert,
           nmove,
           nrem,
           nleast);
    Printf("calls to find=%lu\n", nfind);
    Printf("comparisons=%d\n", sptree_->enqcmps);
#else
    Printf("Turn on COLLECT_TQueue_STATISTICS_ in tqueue.h\n");
#endif
}

void TQueue::spike_stat(double* d) {
#if COLLECT_TQueue_STATISTICS
    d[0] = ninsert;
    d[1] = nmove;
    d[2] = nrem;
// printf("FifoQ spikestat nfenq=%lu nfdeq=%lu nfrem=%lu\n", fifo_->nfenq, fifo_->nfdeq,
// fifo_->nfrem);
#endif
}

TQItem* TQueue::insert(double t, void* d) {
    MUTLOCK
    STAT(ninsert);
    TQItem* i = tpool_->alloc();
    i->data_ = d;
    i->t_ = t;
    i->cnt_ = -1;
    if (t < least_t_nolock()) {
        if (least()) {
            spenq(least(), sptree_);
        }
        least_ = i;
    } else {
        spenq(i, sptree_);
    }
    MUTUNLOCK
    return i;
}

TQItem* TQueue::enqueue_bin(double td, void* d) {
    MUTLOCK
    STAT(ninsert);
    TQItem* i = tpool_->alloc();
    i->data_ = d;
    i->t_ = td;
    binq_->enqueue(td, i);
    MUTUNLOCK
    return i;
}

void TQueue::release(TQItem* q) {
    // if lockable then the pool is internally handles locking
    tpool_->hpfree(q);
}

void TQueue::remove(TQItem* q) {
    MUTLOCK
    STAT(nrem);
    if (q) {
        if (q == least_) {
            if (sptree_->root) {
                least_ = spdeq(&sptree_->root);
            } else {
                least_ = nullptr;
            }
        } else if (q->cnt_ >= 0) {
            binq_->remove(q);
        } else {
            spdelete(q, sptree_);
        }
        tpool_->hpfree(q);
    }
    MUTUNLOCK
}

TQItem* TQueue::atomic_dq(double tt) {
    TQItem* q = 0;
    MUTLOCK
    if (least_ && least_->t_ <= tt) {
        q = least_;
        STAT(nrem);
        if (sptree_->root) {
            least_ = spdeq(&sptree_->root);
        } else {
            least_ = nullptr;
        }
    }
    MUTUNLOCK
    return q;
}

TQItem* TQueue::find(double t) {
    TQItem* q;
    MUTLOCK
    // search only in the  splay tree. if this is a bug then fix it.
    STAT(nfind)
    if (t == least_t_nolock()) {
        q = least();
    } else {
        q = splookup(t, sptree_);
    }
    MUTUNLOCK
    return (q);
}

BinQ::BinQ() {
    nbin_ = 1000;
    bins_ = new TQItem*[nbin_];
    for (int i = 0; i < nbin_; ++i) {
        bins_[i] = 0;
    }
    qpt_ = 0;
    tt_ = 0.;
#if COLLECT_TQueue_STATISTICS
    nfenq = nfdeq = nfrem = 0;
#endif
}

BinQ::~BinQ() {
    for (int i = 0; i < nbin_; ++i) {
        assert(!bins_[i]);
    }
    delete[] bins_;
}

void BinQ::resize(int size) {
    // printf("BinQ::resize from %d to %d\n", nbin_, size);
    int i, j;
    TQItem* q;
    assert(size >= nbin_);
    TQItem** bins = new TQItem*[size];
    for (i = nbin_; i < size; ++i) {
        bins[i] = 0;
    }
    for (i = 0, j = qpt_; i < nbin_; ++i, ++j) {
        if (j >= nbin_) {
            j = 0;
        }
        bins[i] = bins_[j];
        for (q = bins[i]; q; q = q->left_) {
            q->cnt_ = i;
        }
    }
    delete[] bins_;
    bins_ = bins;
    nbin_ = size;
    qpt_ = 0;
}
void BinQ::enqueue(double td, TQItem* q) {
    int idt = (int) ((td - tt_) / nt_dt + 1.e-10);
    if (idt < 0) {
        if (nrn_binq_enqueue_error_handler) {
            (*nrn_binq_enqueue_error_handler)(td, q);
            return;
        } else {
            assert(idt >= 0);
        }
    }
    if (idt >= nbin_) {
        resize(idt + 100);
    }
    // assert (idt < nbin_);
    idt += qpt_;
    if (idt >= nbin_) {
        idt -= nbin_;
    }
    // printf("enqueue idt=%d qpt=%d\n", idt, qpt_);
    assert(idt < nbin_);
    q->cnt_ = idt;  // only for iteration
    q->left_ = bins_[idt];
    bins_[idt] = q;
#if COLLECT_TQueue_STATISTICS
    ++nfenq;
#endif
}
TQItem* BinQ::dequeue() {
    TQItem* q = bins_[qpt_];
    if (q) {
        bins_[qpt_] = q->left_;
#if COLLECT_TQueue_STATISTICS
        ++nfdeq;
#endif
    }
    return q;
}

/** Iterate in ascending bin order starting at current bin **/
TQItem* BinQ::first() {
    for (int i = 0; i < nbin_; ++i) {
        // start at least time qpt_ up to nbin_, and then wrap
        // around to 0 and go up to qpt_
        int j = (qpt_ + i) % nbin_;
        if (bins_[j]) {
            return bins_[j];
        }
    }
    return 0;
}
TQItem* BinQ::next(TQItem* q) {
    if (q->left_) {
        return q->left_;
    }
    // next non-empty bin starting at q->cnt_ + 1, until reach
    // exactly qpt_, possibly wrapping around back to 0 if reach nbin_
    for (int i = (q->cnt_ + 1) % nbin_; i != qpt_; i = (i + 1) % nbin_) {
        if (bins_[i]) {
            return bins_[i];
        }
    }
    return 0;
}

void BinQ::remove(TQItem* q) {
    TQItem *q1, *q2;
    q1 = bins_[q->cnt_];
    if (q1 == q) {
        bins_[q->cnt_] = q->left_;
        return;
    }
    for (q2 = q1->left_; q2; q1 = q2, q2 = q2->left_) {
        if (q2 == q) {
            q1->left_ = q->left_;
            return;
        }
    }
}

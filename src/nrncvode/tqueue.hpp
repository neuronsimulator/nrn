#pragma once

#undef check

#include <assert.h>

#include <pool.hpp>

class TQItem;
using TQItemPool = MutexPool<TQItem>;

// bin queue for the fixed step method for NetCons and PreSyns. Splay tree
// for others.
// fifo for the NetCons and PreSyns with same delay. Splay tree for
// others (especially SelfEvents).
// note that most methods below assume a TQItem is in the splay tree
// For the bin part, only insert_fifo, and remove make sense,
// and forall_callback does the splay tree first and then the bin (so
// not in time order)
// The bin part assumes a fixed step method.

#define COLLECT_TQueue_STATISTICS 1
template <typename T>
struct SPTREE;

class TQItem {
  public:
    TQItem();
    virtual ~TQItem();
    bool check();
    void clear(){};

  public:
    void* data_;
    double t_;
    TQItem* left_;
    TQItem* right_;
    TQItem* parent_;
    int cnt_;  // reused: -1 means it is in the splay tree, >=0 gives bin
};

// helper class for the TQueue (SplayTBinQueue).
class BinQ {
  public:
    BinQ();
    virtual ~BinQ();
    void enqueue(double tt, TQItem*);
    void shift(double tt) {
        assert(!bins_[qpt_]);
        tt_ = tt;
        if (++qpt_ >= nbin_) {
            qpt_ = 0;
        }
    }
    TQItem* top() {
        return bins_[qpt_];
    }
    TQItem* dequeue();
    double tbin() {
        return tt_;
    }
    // for iteration
    TQItem* first();
    TQItem* next(TQItem*);
    void remove(TQItem*);
    void resize(int);
#if COLLECT_TQueue_STATISTICS
  public:
    int nfenq, nfdeq, nfrem;
#endif
  private:
    double tt_;  // time at beginning of qpt_ interval
    int nbin_, qpt_;
    TQItem** bins_;
};

class TQueue {
  public:
    explicit TQueue(TQItemPool*);
    virtual ~TQueue();

    TQItem* least() {
        return least_;
    }
    TQItem* second_least(double t);
    double least_t() {
        if (least_) {
            return least_->t_;
        } else {
            return 1e15;
        }
    }
    TQItem* atomic_dq(double til);
    TQItem* insert(double t, void* data);
    TQItem* enqueue_bin(double t, void* data);
    TQItem* dequeue_bin() {
        return binq_->dequeue();
    }
    void shift_bin(double t) {
        ++nshift_;
        binq_->shift(t);
    }
    double tbin() {
        return binq_->tbin();
    }
    TQItem* top() {
        return binq_->top();
    }
    void release(TQItem*);
    TQItem* find(double t);
    void remove(TQItem*);
    void move(TQItem*, double tnew);
    void move_least(double tnew);
    void print();
    void check(const char* errmess);
    void statistics();
    void spike_stat(double*);
    void forall_callback(void (*)(const TQItem*, int));
    int nshift_;
    void deleteitem(TQItem*);

  public:
    BinQ* binq() {
        return binq_;
    }

  private:
    double least_t_nolock() {
        if (least_) {
            return least_->t_;
        } else {
            return 1e15;
        }
    }
    void move_least_nolock(double tnew);
    SPTREE<TQItem>* sptree_;
    BinQ* binq_;
    TQItem* least_;
    TQItemPool* tpool_;
#if COLLECT_TQueue_STATISTICS
    unsigned long ninsert, nrem, nleast, nbal, ncmplxrem;
    unsigned long ncompare, nleastsrch, nfind, nfindsrch, nmove, nfastmove;
#endif
};

class SelfQueue {  // not really a queue but a doubly linked list for fast
  public:          // insertion, deletion, iteration
    explicit SelfQueue(TQItemPool*);
    virtual ~SelfQueue();
    TQItem* insert(void*);
    void* remove(TQItem*);
    void remove_all();
    TQItem* first() {
        return head_;
    }
    TQItem* next(TQItem* q) {
        return q->right_;
    }

  private:
    TQItem* head_;
    TQItemPool* tpool_;
};
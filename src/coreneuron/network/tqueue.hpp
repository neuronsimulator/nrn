/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

/*
**  SPTREE:  The following type declarations provide the binary tree
**  representation of event-sets or priority queues needed by splay trees
**
**  assumes that data and datb will be provided by the application
**  to hold all application specific information
**
**  assumes that key will be provided by the application, comparable
**  with the compare function applied to the addresses of two keys.
*/
// bin queue for the fixed step method for NetCons and PreSyns. Splay tree
// for others.
// fifo for the NetCons and PreSyns with same delay. Splay tree for
// others (especially SelfEvents).
// note that most methods below assume a TQItem is in the splay tree
// For the bin part, only insert_fifo, and remove make sense,
// The bin part assumes a fixed step method.

#include <cstdio>
#include <cassert>
#include <queue>
#include <vector>
#include <map>
#include <utility>

namespace coreneuron {
#define STRCMP(a, b) (a - b)

class TQItem;
#define SPBLK     TQItem
#define leftlink  left_
#define rightlink right_
#define uplink    parent_
#define cnt       cnt_
#define key       t_

struct SPTREE {
    SPBLK* root; /* root node */

    /* Statistics, not strictly necessary, but handy for tuning  */
    int enqcmps; /* compares in spenq */
};

#define spinit   sptq_spinit
#define spenq    sptq_spenq
#define spdeq    sptq_spdeq
#define splay    sptq_splay
#define sphead   sptq_sphead
#define spdelete sptq_spdelete

extern void spinit(SPTREE*);           /* init tree */
extern SPBLK* spenq(SPBLK*, SPTREE*);  /* insert item into the tree */
extern SPBLK* spdeq(SPBLK**);          /* return and remove lowest item in subtree */
extern void splay(SPBLK*, SPTREE*);    /* reorganize tree */
extern SPBLK* sphead(SPTREE*);         /* return first node in tree */
extern void spdelete(SPBLK*, SPTREE*); /* delete node from tree */

struct DiscreteEvent;
class TQItem {
  public:
    DiscreteEvent* data_ = nullptr;
    double t_ = 0;
    TQItem* left_ = nullptr;
    TQItem* right_ = nullptr;
    TQItem* parent_ = nullptr;
    int cnt_ = 0;  // reused: -1 means it is in the splay tree, >=0 gives bin
};

using TQPair = std::pair<double, TQItem*>;

struct less_time {
    bool operator()(const TQPair& x, const TQPair& y) const {
        return x.first > y.first;
    }
};

// helper class for the TQueue (SplayTBinQueue).
class BinQ {
  public:
    BinQ();
    ~BinQ();
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

  private:
    double tt_;  // time at beginning of qpt_ interval
    int nbin_, qpt_;
    TQItem** bins_;
    std::vector<std::vector<TQItem*>> vec_bins;
};

enum container { spltree, pq_que };

template <container C = spltree>
class TQueue {
  public:
    TQueue();
    ~TQueue();

    inline TQItem* least() {
        return least_;
    }
    inline TQItem* insert(double t, DiscreteEvent* data);
    inline TQItem* enqueue_bin(double t, DiscreteEvent* data);
    inline TQItem* dequeue_bin() {
        return binq_->dequeue();
    }
    inline void shift_bin(double _t_) {
        ++nshift_;
        binq_->shift(_t_);
    }
    inline TQItem* top() {
        return binq_->top();
    }

    inline TQItem* atomic_dq(double til);
    inline void remove(TQItem*);
    inline void move(TQItem*, double tnew);
    int nshift_;

    /// Priority queue of vectors for queuing the events. enqueuing for move() and
    /// move_least_nolock() is not implemented
    std::priority_queue<TQPair, std::vector<TQPair>, less_time> pq_que_;
    /// Types of queuing statistics
    enum qtype { enq = 0, spike, ite, deq };

  private:
    double least_t_nolock() {
        if (least_) {
            return least_->t_;
        } else {
            return 1e15;
        }
    }
    void move_least_nolock(double tnew);
    SPTREE* sptree_;

  public:
    BinQ* binq_;

  private:
    TQItem* least_;
    TQPair make_TQPair(TQItem* p) {
        return TQPair(p->t_, p);
    }
};
}  // namespace coreneuron
#include "coreneuron/network/tqueue.ipp"

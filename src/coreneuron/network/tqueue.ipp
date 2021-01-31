/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef tqueue_ipp_
#define tqueue_ipp_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/network/tqueue.hpp"

namespace coreneuron {
// splay tree + bin queue limited to fixed step method
// for event-sets or priority queues
// this starts from the sptqueue.cpp file and adds a bin queue

/* Derived from David Brower's c translation of pascal code by
Douglas Jones.
*/
/* The original c code is included from this file but note that instead
of struct _spblk, we are really using TQItem
*/

template <container C>
TQueue<C>::TQueue() {
    MUTCONSTRUCT(0)
    nshift_ = 0;
    sptree_ = new SPTREE;
    spinit(sptree_);
    binq_ = new BinQ;
    least_ = 0;
}

template <container C>
TQueue<C>::~TQueue() {
    SPBLK *q, *q2;
    /// Clear the binq
    for (q = binq_->first(); q; q = q2) {
        q2 = binq_->next(q);
        remove(q);  /// Potentially dereferences freed pointer this->sptree_
    }
    delete binq_;

    if (least_) {
        delete least_;
        least_ = nullptr;
    }

    /// Clear the splay tree
    while ((q = spdeq(&sptree_->root)) != nullptr) {
        delete q;
    }
    delete sptree_;

    /// Clear the priority queue
    while (pq_que_.size()) {
        delete pq_que_.top().second;
        pq_que_.pop();
    }

    MUTDESTRUCT
}

template <container C>
TQItem* TQueue<C>::enqueue_bin(double td, void* d) {
    MUTLOCK
    TQItem* i = new TQItem;
    i->data_ = d;
    i->t_ = td;
    binq_->enqueue(td, i);
    MUTUNLOCK
    return i;
}

/// Splay tree priority queue implementation
template <>
inline void TQueue<spltree>::move_least_nolock(double tnew) {
    TQItem* b = least();
    if (b) {
        b->t_ = tnew;
        TQItem* nl;
        nl = sphead(sptree_);
        if (nl && (tnew > nl->t_)) {
            least_ = spdeq(&sptree_->root);
            spenq(b, sptree_);
        }
    }
}

/// STL priority queue implementation
template <>
inline void TQueue<pq_que>::move_least_nolock(double tnew) {
    TQItem* b = least();
    if (b) {
        b->t_ = tnew;
        TQItem* nl;
        nl = pq_que_.top().second;
        if (nl && (tnew > nl->t_)) {
            least_ = nl;
            pq_que_.pop();
            pq_que_.push(make_TQPair(b));
        }
    }
}

/// Splay tree priority queue implementation
template <>
inline void TQueue<spltree>::move(TQItem* i, double tnew) {
    MUTLOCK
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

/// STL priority queue implementation
template <>
inline void TQueue<pq_que>::move(TQItem* i, double tnew) {
    MUTLOCK
    if (i == least_) {
        move_least_nolock(tnew);
    } else if (tnew < least_->t_) {
        TQItem* qmove = new TQItem;
        qmove->data_ = i->data_;
        qmove->t_ = tnew;
        qmove->cnt_ = i->cnt_;
        i->t_ = -1.;
        pq_que_.push(make_TQPair(least_));
        least_ = qmove;
    } else {
        TQItem* qmove = new TQItem;
        qmove->data_ = i->data_;
        qmove->t_ = tnew;
        qmove->cnt_ = i->cnt_;
        i->t_ = -1.;
        pq_que_.push(make_TQPair(qmove));
    }
    MUTUNLOCK
}

/// Splay tree priority queue implementation
template <>
inline TQItem* TQueue<spltree>::insert(double tt, void* d) {
    MUTLOCK
    TQItem* i = new TQItem;
    i->data_ = d;
    i->t_ = tt;
    i->cnt_ = -1;
    if (tt < least_t_nolock()) {
        if (least_) {
            /// Probably storing both time and event which has the time is redundant, but the event
            /// is then returned
            /// to the upper level call stack function. If we were to eliminate i->t_ and i->cnt_
            /// fields,
            /// we need to make sure we are not braking anything.
            spenq(least_, sptree_);
        }
        least_ = i;
    } else {
        spenq(i, sptree_);
    }
    MUTUNLOCK
    return i;
}

/// STL priority queue implementation
template <>
inline TQItem* TQueue<pq_que>::insert(double tt, void* d) {
    MUTLOCK
    TQItem* i = new TQItem;
    i->data_ = d;
    i->t_ = tt;
    i->cnt_ = -1;
    if (tt < least_t_nolock()) {
        if (least_) {
            /// Probably storing both time and event which has the time is redundant, but the event
            /// is then returned
            /// to the upper level call stack function. If we were to eliminate i->t_ and i->cnt_
            /// fields,
            /// we need to make sure we are not braking anything.
            pq_que_.push(make_TQPair(least_));
        }
        least_ = i;
    } else {
        pq_que_.push(make_TQPair(i));
    }
    MUTUNLOCK
    return i;
}

/// Splay tree priority queue implementation
template <>
inline void TQueue<spltree>::remove(TQItem* q) {
    MUTLOCK
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
        delete q;
    }
    MUTUNLOCK
}

/// STL priority queue implementation
template <>
inline void TQueue<pq_que>::remove(TQItem* q) {
    MUTLOCK
    if (q) {
        if (q == least_) {
            if (pq_que_.size()) {
                least_ = pq_que_.top().second;
                pq_que_.pop();
            } else {
                least_ = nullptr;
            }
        } else {
            q->t_ = -1.;
        }
    }
    MUTUNLOCK
}

/// Splay tree priority queue implementation
template <>
inline TQItem* TQueue<spltree>::atomic_dq(double tt) {
    TQItem* q = nullptr;
    MUTLOCK
    if (least_ && least_->t_ <= tt) {
        q = least_;
        if (sptree_->root) {
            least_ = spdeq(&sptree_->root);
        } else {
            least_ = nullptr;
        }
    }
    MUTUNLOCK
    return q;
}

/// STL priority queue implementation
template <>
inline TQItem* TQueue<pq_que>::atomic_dq(double tt) {
    TQItem* q = nullptr;
    MUTLOCK
    if (least_ && least_->t_ <= tt) {
        q = least_;
        //        int qsize = pq_que_.size();
        //        printf("map size: %d\n", msize);
        /// This while loop is to delete events whose times have been moved with the ::move
        /// function,
        /// but in fact events were left in the queue since the only function available is pop
        while (pq_que_.size() && pq_que_.top().second->t_ < 0.) {
            delete pq_que_.top().second;
            pq_que_.pop();
        }
        if (pq_que_.size()) {
            least_ = pq_que_.top().second;
            pq_que_.pop();
        } else {
            least_ = nullptr;
        }
    }
    MUTUNLOCK
    return q;
}
}  // namespace coreneuron
#endif

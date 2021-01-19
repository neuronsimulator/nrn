/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

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

TQItem::TQItem() {
    left_ = 0;
    right_ = 0;
    parent_ = 0;
}

BinQ::BinQ() {
    nbin_ = 1000;
    bins_ = new TQItem*[nbin_];
    for (int i = 0; i < nbin_; ++i) {
        bins_[i] = 0;
    }
    qpt_ = 0;
    tt_ = 0.;
}

BinQ::~BinQ() {
    for (int i = 0; i < nbin_; ++i) {
        assert(!bins_[i]);
    }
    delete[] bins_;
    vec_bins.clear();
}

void BinQ::resize(int size) {
    // printf("BinQ::resize from %d to %d\n", nbin_, size);
    assert(size >= nbin_);
    TQItem** bins = new TQItem*[size];
    for (int i = nbin_; i < size; ++i) {
        bins[i] = 0;
    }
    for (int i = 0, j = qpt_; i < nbin_; ++i, ++j) {
        if (j >= nbin_) {
            j = 0;
        }
        bins[i] = bins_[j];
        for (auto q = bins[i]; q; q = q->left_) {
            q->cnt_ = i;
        }
    }
    delete[] bins_;
    bins_ = bins;
    nbin_ = size;
    qpt_ = 0;
}
void BinQ::enqueue(double td, TQItem* q) {
    int idt = (int)((td - tt_) * rev_dt + 1.e-10);
    assert(idt >= 0);
    if (idt >= nbin_) {
        resize(idt + 1000);
    }
    // assert (idt < nbin_);
    idt += qpt_;
    if (idt >= nbin_) {
        idt -= nbin_;
    }
    // printf("enqueue: idt=%d qpt=%d nbin_=%d\n", idt, qpt_, nbin_);
    assert(idt < nbin_);
    q->cnt_ = idt;  // only for iteration
    q->left_ = bins_[idt];
    bins_[idt] = q;
}
TQItem* BinQ::dequeue() {
    TQItem* q = bins_[qpt_];
    if (q) {
        bins_[qpt_] = q->left_;
    }
    return q;
}

TQItem* BinQ::first() {
    for (int i = 0; i < nbin_; ++i) {
        if (bins_[i]) {
            return bins_[i];
        }
    }
    return 0;
}
TQItem* BinQ::next(TQItem* q) {
    if (q->left_) {
        return q->left_;
    }
    for (int i = q->cnt_ + 1; i < nbin_; ++i) {
        if (bins_[i]) {
            return bins_[i];
        }
    }
    return 0;
}

void BinQ::remove(TQItem* q) {
    TQItem* q1 = bins_[q->cnt_];
    if (q1 == q) {
        bins_[q->cnt_] = q->left_;
        return;
    }
    for (TQItem* q2 = q1->left_; q2; q1 = q2, q2 = q2->left_) {
        if (q2 == q) {
            q1->left_ = q->left_;
            return;
        }
    }
}

//#include "coreneuron/nrniv/sptree.h"

/*
 *  The following code implements the basic operations on
 *  an event-set or priority-queue implemented using splay trees:
 *
Hines changed to void spinit(SPTREE**) for use with TQueue.
 *  SPTREE *spinit( compare )	Make a new tree
 *  SPBLK *spenq( n, q )	Insert n in q after all equal keys.
 *  SPBLK *spdeq( np )		Return first key under *np, removing it.
 *  void splay( n, q )		n (already in q) becomes the root.
 *  int n = sphead( q )         n is the head item in q (not removed).
 *  spdelete( n, q )		n is removed from q.
 *
 *  In the above, n points to an SPBLK type, while q points to an
 *  SPTREE.
 *
 *  The implementation used here is based on the implementation
 *  which was used in the tests of splay trees reported in:
 *
 *    An Empirical Comparison of Priority-Queue and Event-Set Implementations,
 *	by Douglas W. Jones, Comm. ACM 29, 4 (Apr. 1986) 300-311.
 *
 *  The changes made include the addition of the enqprior
 *  operation and the addition of up-links to allow for the splay
 *  operation.  The basic splay tree algorithms were originally
 *  presented in:
 *
 *	Self Adjusting Binary Trees,
 *		by D. D. Sleator and R. E. Tarjan,
 *			Proc. ACM SIGACT Symposium on Theory
 *			of Computing (Boston, Apr 1983) 235-245.
 *
 *  The enq and enqprior routines use variations on the
 *  top-down splay operation, while the splay routine is bottom-up.
 *  All are coded for speed.
 *
 *  Written by:
 *    Douglas W. Jones
 *
 *  Translated to C by:
 *    David Brower, daveb@rtech.uucp
 *
 * Thu Oct  6 12:11:33 PDT 1988 (daveb) Fixed spdeq, which was broken
 *	handling one-node trees.  I botched the pascal translation of
 *	a VAR parameter.
 */

/*----------------
 *
 * spinit() -- initialize an empty splay tree
 *
 */
void spinit(SPTREE* q) {
    q->enqcmps = 0;
    q->root = nullptr;
}

/*----------------
 *
 *  spenq() -- insert item in a tree.
 *
 *  put n in q after all other nodes with the same key; when this is
 *  done, n will be the root of the splay tree representing q, all nodes
 *  in q with keys less than or equal to that of n will be in the
 *  left subtree, all with greater keys will be in the right subtree;
 *  the tree is split into these subtrees from the top down, with rotations
 *  performed along the way to shorten the left branch of the right subtree
 *  and the right branch of the left subtree
 */
SPBLK* spenq(SPBLK* n, SPTREE* q) {
    SPBLK* left;  /* the rightmost node in the left tree */
    SPBLK* right; /* the leftmost node in the right tree */
    SPBLK* next;  /* the root of the unsplit part */
    SPBLK* temp;

    double key;

    n->uplink = nullptr;
    next = q->root;
    q->root = n;
    if (next == nullptr) /* trivial enq */
    {
        n->leftlink = nullptr;
        n->rightlink = nullptr;
    } else /* difficult enq */
    {
        key = n->key;
        left = n;
        right = n;

        /* n's left and right children will hold the right and left
       splayed trees resulting from splitting on n->key;
       note that the children will be reversed! */

        q->enqcmps++;
        if (STRCMP(next->key, key) > 0)
            goto two;

    one: /* assert next->key <= key */

        do /* walk to the right in the left tree */
        {
            temp = next->rightlink;
            if (temp == nullptr) {
                left->rightlink = next;
                next->uplink = left;
                right->leftlink = nullptr;
                goto done; /* job done, entire tree split */
            }

            q->enqcmps++;
            if (STRCMP(temp->key, key) > 0) {
                left->rightlink = next;
                next->uplink = left;
                left = next;
                next = temp;
                goto two; /* change sides */
            }

            next->rightlink = temp->leftlink;
            if (temp->leftlink != nullptr)
                temp->leftlink->uplink = next;
            left->rightlink = temp;
            temp->uplink = left;
            temp->leftlink = next;
            next->uplink = temp;
            left = temp;
            next = temp->rightlink;
            if (next == nullptr) {
                right->leftlink = nullptr;
                goto done; /* job done, entire tree split */
            }

            q->enqcmps++;

        } while (STRCMP(next->key, key) <= 0); /* change sides */

    two: /* assert next->key > key */

        do /* walk to the left in the right tree */
        {
            temp = next->leftlink;
            if (temp == nullptr) {
                right->leftlink = next;
                next->uplink = right;
                left->rightlink = nullptr;
                goto done; /* job done, entire tree split */
            }

            q->enqcmps++;
            if (STRCMP(temp->key, key) <= 0) {
                right->leftlink = next;
                next->uplink = right;
                right = next;
                next = temp;
                goto one; /* change sides */
            }
            next->leftlink = temp->rightlink;
            if (temp->rightlink != nullptr)
                temp->rightlink->uplink = next;
            right->leftlink = temp;
            temp->uplink = right;
            temp->rightlink = next;
            next->uplink = temp;
            right = temp;
            next = temp->leftlink;
            if (next == nullptr) {
                left->rightlink = nullptr;
                goto done; /* job done, entire tree split */
            }

            q->enqcmps++;

        } while (STRCMP(next->key, key) > 0); /* change sides */

        goto one;

    done: /* split is done, branches of n need reversal */

        temp = n->leftlink;
        n->leftlink = n->rightlink;
        n->rightlink = temp;
    }

    return (n);

} /* spenq */

/*----------------
 *
 *  spdeq() -- return and remove head node from a subtree.
 *
 *  remove and return the head node from the node set; this deletes
 *  (and returns) the leftmost node from q, replacing it with its right
 *  subtree (if there is one); on the way to the leftmost node, rotations
 *  are performed to shorten the left branch of the tree
 */
SPBLK* spdeq(SPBLK** np) /* pointer to a node pointer */

{
    SPBLK* deq;        /* one to return */
    SPBLK* next;       /* the next thing to deal with */
    SPBLK* left;       /* the left child of next */
    SPBLK* farleft;    /* the left child of left */
    SPBLK* farfarleft; /* the left child of farleft */

    if (np == nullptr || *np == nullptr) {
        deq = nullptr;
    } else {
        next = *np;
        left = next->leftlink;
        if (left == nullptr) {
            deq = next;
            *np = next->rightlink;

            if (*np != nullptr)
                (*np)->uplink = nullptr;

        } else
            for (;;) /* left is not null */
            {
                /* next is not it, left is not nullptr, might be it */
                farleft = left->leftlink;
                if (farleft == nullptr) {
                    deq = left;
                    next->leftlink = left->rightlink;
                    if (left->rightlink != nullptr)
                        left->rightlink->uplink = next;
                    break;
                }

                /* next, left are not it, farleft is not nullptr, might be it */
                farfarleft = farleft->leftlink;
                if (farfarleft == nullptr) {
                    deq = farleft;
                    left->leftlink = farleft->rightlink;
                    if (farleft->rightlink != nullptr)
                        farleft->rightlink->uplink = left;
                    break;
                }

                /* next, left, farleft are not it, rotate */
                next->leftlink = farleft;
                farleft->uplink = next;
                left->leftlink = farleft->rightlink;
                if (farleft->rightlink != nullptr)
                    farleft->rightlink->uplink = left;
                farleft->rightlink = left;
                left->uplink = farleft;
                next = farleft;
                left = farfarleft;
            }
    }

    return (deq);

} /* spdeq */

/*----------------
 *
 *  splay() -- reorganize the tree.
 *
 *  the tree is reorganized so that n is the root of the
 *  splay tree representing q; results are unpredictable if n is not
 *  in q to start with; q is split from n up to the old root, with all
 *  nodes to the left of n ending up in the left subtree, and all nodes
 *  to the right of n ending up in the right subtree; the left branch of
 *  the right subtree and the right branch of the left subtree are
 *  shortened in the process
 *
 *  this code assumes that n is not nullptr and is in q; it can sometimes
 *  detect n not in q and complain
 */

void splay(SPBLK* n, SPTREE* q) {
    SPBLK* up;     /* points to the node being dealt with */
    SPBLK* prev;   /* a descendent of up, already dealt with */
    SPBLK* upup;   /* the parent of up */
    SPBLK* upupup; /* the grandparent of up */
    SPBLK* left;   /* the top of left subtree being built */
    SPBLK* right;  /* the top of right subtree being built */

    left = n->leftlink;
    right = n->rightlink;
    prev = n;
    up = prev->uplink;

    while (up != nullptr) {
        /* walk up the tree towards the root, splaying all to the left of
       n into the left subtree, all to right into the right subtree */

        upup = up->uplink;
        if (up->leftlink == prev) /* up is to the right of n */
        {
            if (upup != nullptr && upup->leftlink == up) /* rotate */
            {
                upupup = upup->uplink;
                upup->leftlink = up->rightlink;
                if (upup->leftlink != nullptr)
                    upup->leftlink->uplink = upup;
                up->rightlink = upup;
                upup->uplink = up;
                if (upupup == nullptr)
                    q->root = up;
                else if (upupup->leftlink == upup)
                    upupup->leftlink = up;
                else
                    upupup->rightlink = up;
                up->uplink = upupup;
                upup = upupup;
            }
            up->leftlink = right;
            if (right != nullptr)
                right->uplink = up;
            right = up;

        } else /* up is to the left of n */
        {
            if (upup != nullptr && upup->rightlink == up) /* rotate */
            {
                upupup = upup->uplink;
                upup->rightlink = up->leftlink;
                if (upup->rightlink != nullptr)
                    upup->rightlink->uplink = upup;
                up->leftlink = upup;
                upup->uplink = up;
                if (upupup == nullptr)
                    q->root = up;
                else if (upupup->rightlink == upup)
                    upupup->rightlink = up;
                else
                    upupup->leftlink = up;
                up->uplink = upupup;
                upup = upupup;
            }
            up->rightlink = left;
            if (left != nullptr)
                left->uplink = up;
            left = up;
        }
        prev = up;
        up = upup;
    }

#ifdef DEBUG
    if (q->root != prev) {
        /*	fprintf(stderr, " *** bug in splay: n not in q *** " ); */
        abort();
    }
#endif

    n->leftlink = left;
    n->rightlink = right;
    if (left != nullptr)
        left->uplink = n;
    if (right != nullptr)
        right->uplink = n;
    q->root = n;
    n->uplink = nullptr;

} /* splay */

/*----------------
 *
 * sphead() --  return the "lowest" element in the tree.
 *
 *      returns a reference to the head event in the event-set q,
 *      represented as a splay tree; q->root ends up pointing to the head
 *      event, and the old left branch of q is shortened, as if q had
 *      been splayed about the head element; this is done by dequeueing
 *      the head and then making the resulting queue the right son of
 *      the head returned by spdeq; an alternative is provided which
 *      avoids splaying but just searches for and returns a pointer to
 *      the bottom of the left branch
 */
SPBLK* sphead(SPTREE* q) {
    SPBLK* x;

    /* splay version, good amortized bound */
    x = spdeq(&q->root);
    if (x != nullptr) {
        x->rightlink = q->root;
        x->leftlink = nullptr;
        x->uplink = nullptr;
        if (q->root != nullptr)
            q->root->uplink = x;
    }
    q->root = x;

    /* alternative version, bad amortized bound,
       but faster on the average */

    return (x);

} /* sphead */

/*----------------
 *
 * spdelete() -- Delete node from a tree.
 *
 *	n is deleted from q; the resulting splay tree has been splayed
 *	around its new root, which is the successor of n
 *
 */
void spdelete(SPBLK* n, SPTREE* q) {
    SPBLK* x;

    splay(n, q);
    x = spdeq(&q->root->rightlink);
    if (x == nullptr) /* empty right subtree */
    {
        q->root = q->root->leftlink;
        if (q->root)
            q->root->uplink = nullptr;
    } else /* non-empty right subtree */
    {
        x->uplink = nullptr;
        x->leftlink = q->root->leftlink;
        x->rightlink = q->root->rightlink;
        if (x->leftlink != nullptr)
            x->leftlink->uplink = x;
        if (x->rightlink != nullptr)
            x->rightlink->uplink = x;
        q->root = x;
    }

} /* spdelete */
}  // namespace coreneuron

/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrniv/tqueue.h"

#if COLLECT_TQueue_STATISTICS
#define STAT(arg) ++arg;
#else
#define STAT(arg) /**/
#endif

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

TQItem::~TQItem() {
}

TQueue::TQueue() {
    MUTCONSTRUCT(0)
    nshift_ = 0;
    sptree_ = new SPTREE;
    spinit(sptree_);
    binq_ = new BinQ;
    least_ = 0;

#if COLLECT_TQueue_STATISTICS
    nmove = ninsert = nrem = nleast = nbal = ncmplxrem = 0;
    nfastmove = ncompare = nleastsrch = nfind = nfindsrch = 0;
#endif
}

TQueue::~TQueue() {
    SPBLK* q, *q2;
    while((q = spdeq(&sptree_->root)) != NULL) {
        delete q;
    }
    delete sptree_;
    for (q = binq_->first(); q; q = q2) {
        q2 = binq_->next(q);
        remove(q); /// Potentially dereferences freed pointer this->sptree_
    }
    delete binq_;
    MUTDESTRUCT
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
    }else if (tnew < least_->t_) {
        spdelete(i, sptree_);
        i->t_ = tnew;
        spenq(least_, sptree_);
        least_ = i;
    }else{
        spdelete(i, sptree_);
        i->t_ = tnew;
        spenq(i, sptree_);
    }
    MUTUNLOCK
}

void TQueue::statistics() {
#if COLLECT_TQueue_STATISTICS
    printf("insertions=%lu  moves=%lu removals=%lu calls to least=%lu\n",
        ninsert, nmove, nrem, nleast);
    printf("calls to find=%lu\n",
        nfind);
    printf("comparisons=%d\n",
        sptree_->enqcmps);
#else
    printf("Turn on COLLECT_TQueue_STATISTICS_ in tqueue.h\n");
#endif
}

TQItem* TQueue::insert(double tt, void* d) {
    MUTLOCK
    STAT(ninsert);
    TQItem* i = new TQItem;
    i->data_ = d;
    i->t_ = tt;
    i->cnt_ = -1;
    if (tt < least_t_nolock()) {
        if (least()) {
            spenq(least(), sptree_);
        }
        least_ = i;
    }else{
        spenq(i, sptree_);
    }
    MUTUNLOCK
    return i;
}

void TQueue::remove(TQItem* q) {
    MUTLOCK
    STAT(nrem);
    if (q) {
        if (q == least_) {
            if (sptree_->root) {
                least_ = spdeq(&sptree_->root);
            }else{
                least_ = NULL;
            }
        }else if (q->cnt_ >= 0) {
            binq_->remove(q);
        }else{
            spdelete(q, sptree_);
        }
        delete q;
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
        }else{
            least_ = NULL;
        }
    }
    MUTUNLOCK
    return q;
}

BinQ::BinQ() {
    nbin_ = 1000;
    bins_ = new TQItem*[nbin_];
    for (int i=0; i < nbin_; ++i) { bins_[i] = 0; }
    qpt_ = 0;
    tt_ = 0.;
#if COLLECT_TQueue_STATISTICS
    nfenq = 0;
#endif
}

BinQ::~BinQ() {
    for (int i=0; i < nbin_; ++i) {
        assert(!bins_[i]);
    }
    delete [] bins_;
}

void BinQ::resize(int size) {
    //printf("BinQ::resize from %d to %d\n", nbin_, size);
    int i, j;
    TQItem* q;
    assert(size >= nbin_);
    TQItem** bins = new TQItem*[size];
    for (i=nbin_; i < size; ++i) { bins[i] = 0; }
    for (i=0, j=qpt_; i < nbin_; ++i, ++j) {
        if (j >= nbin_) { j = 0; }
        bins[i] = bins_[j];
        for (q = bins[i]; q; q = q->left_) {
            q->cnt_ = i;
        }
    }
    delete [] bins_;
    bins_ = bins;
    nbin_ = size;
    qpt_ = 0;
}
void BinQ::enqueue(double td, TQItem* q) {
    int idt = (int)((td - tt_)/nrn_threads->_dt + 1.e-10);
    assert(idt >= 0);
    if (idt >= nbin_) {
        resize(idt + 100);
    }
    //assert (idt < nbin_);
    idt += qpt_;
    if (idt >= nbin_) { idt -= nbin_; }
//printf("enqueue idt=%d qpt=%d\n", idt, qpt_);
    assert (idt < nbin_);
    q->cnt_ = idt; // only for iteration
    q->left_ = bins_[idt];
    bins_[idt] = q;
#if COLLECT_TQueue_STATISTICS
    ++nfenq;
#endif
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
    if (q->left_) { return q->left_; }
    for (int i = q->cnt_ + 1; i < nbin_; ++i) {
        if (bins_[i]) {
            return bins_[i];
        }
    }
    return 0;
}

void BinQ::remove(TQItem* q) {
    TQItem* q1, *q2;
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
void
spinit(SPTREE* q)
{
    q->enqcmps = 0;
    q->root = NULL;
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
SPBLK *
spenq( SPBLK* n, SPTREE* q )
{
    SPBLK * left;	/* the rightmost node in the left tree */
    SPBLK * right;	/* the leftmost node in the right tree */
    SPBLK * next;	/* the root of the unsplit part */
    SPBLK * temp;

    double key;
#if STRCMP_DEF
    int Sct;		/* Strcmp value */
#endif

    n->uplink = NULL;
    next = q->root;
    q->root = n;
    if( next == NULL )	/* trivial enq */
    {
        n->leftlink = NULL;
        n->rightlink = NULL;
    }
    else		/* difficult enq */
    {
        key = n->key;
        left = n;
        right = n;

        /* n's left and right children will hold the right and left
       splayed trees resulting from splitting on n->key;
       note that the children will be reversed! */

    q->enqcmps++;
        if ( STRCMP( next->key, key ) > 0 )
        goto two;

    one:	/* assert next->key <= key */

    do	/* walk to the right in the left tree */
    {
            temp = next->rightlink;
            if( temp == NULL )
        {
                left->rightlink = next;
                next->uplink = left;
                right->leftlink = NULL;
                goto done;	/* job done, entire tree split */
            }

        q->enqcmps++;
            if( STRCMP( temp->key, key ) > 0 )
        {
                left->rightlink = next;
                next->uplink = left;
                left = next;
                next = temp;
                goto two;	/* change sides */
            }

            next->rightlink = temp->leftlink;
            if( temp->leftlink != NULL )
            temp->leftlink->uplink = next;
            left->rightlink = temp;
            temp->uplink = left;
            temp->leftlink = next;
            next->uplink = temp;
            left = temp;
            next = temp->rightlink;
            if( next == NULL )
        {
                right->leftlink = NULL;
                goto done;	/* job done, entire tree split */
            }

        q->enqcmps++;

    } while( STRCMP( next->key, key ) <= 0 );	/* change sides */

    two:	/* assert next->key > key */

    do	/* walk to the left in the right tree */
    {
            temp = next->leftlink;
            if( temp == NULL )
        {
                right->leftlink = next;
                next->uplink = right;
                left->rightlink = NULL;
                goto done;	/* job done, entire tree split */
            }

        q->enqcmps++;
            if( STRCMP( temp->key, key ) <= 0 )
        {
                right->leftlink = next;
                next->uplink = right;
                right = next;
                next = temp;
                goto one;	/* change sides */
            }
            next->leftlink = temp->rightlink;
            if( temp->rightlink != NULL )
            temp->rightlink->uplink = next;
            right->leftlink = temp;
            temp->uplink = right;
            temp->rightlink = next;
            next->uplink = temp;
            right = temp;
            next = temp->leftlink;
            if( next == NULL )
        {
                left->rightlink = NULL;
                goto done;	/* job done, entire tree split */
            }

        q->enqcmps++;

    } while( STRCMP( next->key, key ) > 0 );	/* change sides */

        goto one;

    done:	/* split is done, branches of n need reversal */

        temp = n->leftlink;
        n->leftlink = n->rightlink;
        n->rightlink = temp;
    }

    return( n );

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
SPBLK *
spdeq( SPBLK** np ) /* pointer to a node pointer */

{
    SPBLK * deq;		/* one to return */
    SPBLK * next;       	/* the next thing to deal with */
    SPBLK * left;      	/* the left child of next */
    SPBLK * farleft;		/* the left child of left */
    SPBLK * farfarleft;	/* the left child of farleft */

    if( np == NULL || *np == NULL )
    {
        deq = NULL;
    }
    else
    {
        next = *np;
        left = next->leftlink;
        if( left == NULL )
    {
            deq = next;
            *np = next->rightlink;

            if( *np != NULL )
        (*np)->uplink = NULL;

        }
    else for(;;)	/* left is not null */
    {
            /* next is not it, left is not NULL, might be it */
            farleft = left->leftlink;
            if( farleft == NULL )
        {
                deq = left;
                next->leftlink = left->rightlink;
                if( left->rightlink != NULL )
            left->rightlink->uplink = next;
        break;
            }

            /* next, left are not it, farleft is not NULL, might be it */
            farfarleft = farleft->leftlink;
            if( farfarleft == NULL )
        {
                deq = farleft;
                left->leftlink = farleft->rightlink;
                if( farleft->rightlink != NULL )
            farleft->rightlink->uplink = left;
        break;
            }

            /* next, left, farleft are not it, rotate */
            next->leftlink = farleft;
            farleft->uplink = next;
            left->leftlink = farleft->rightlink;
            if( farleft->rightlink != NULL )
        farleft->rightlink->uplink = left;
            farleft->rightlink = left;
            left->uplink = farleft;
            next = farleft;
            left = farfarleft;
    }
    }

    return( deq );

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
 *  this code assumes that n is not NULL and is in q; it can sometimes
 *  detect n not in q and complain
 */

void
splay( SPBLK* n, SPTREE* q )
{
    SPBLK * up;	/* points to the node being dealt with */
    SPBLK * prev;	/* a descendent of up, already dealt with */
    SPBLK * upup;	/* the parent of up */
    SPBLK * upupup;	/* the grandparent of up */
    SPBLK * left;	/* the top of left subtree being built */
    SPBLK * right;	/* the top of right subtree being built */

    left = n->leftlink;
    right = n->rightlink;
    prev = n;
    up = prev->uplink;

    while( up != NULL )
    {
        /* walk up the tree towards the root, splaying all to the left of
       n into the left subtree, all to right into the right subtree */

        upup = up->uplink;
        if( up->leftlink == prev )	/* up is to the right of n */
    {
            if( upup != NULL && upup->leftlink == up )  /* rotate */
        {
                upupup = upup->uplink;
                upup->leftlink = up->rightlink;
                if( upup->leftlink != NULL )
            upup->leftlink->uplink = upup;
                up->rightlink = upup;
                upup->uplink = up;
                if( upupup == NULL )
            q->root = up;
        else if( upupup->leftlink == upup )
            upupup->leftlink = up;
        else
            upupup->rightlink = up;
                up->uplink = upupup;
                upup = upupup;
            }
            up->leftlink = right;
            if( right != NULL )
        right->uplink = up;
            right = up;

        }
    else				/* up is to the left of n */
    {
            if( upup != NULL && upup->rightlink == up )	/* rotate */
        {
                upupup = upup->uplink;
                upup->rightlink = up->leftlink;
                if( upup->rightlink != NULL )
            upup->rightlink->uplink = upup;
                up->leftlink = upup;
                upup->uplink = up;
                if( upupup == NULL )
            q->root = up;
        else if( upupup->rightlink == upup )
            upupup->rightlink = up;
        else
            upupup->leftlink = up;
                up->uplink = upupup;
                upup = upupup;
            }
            up->rightlink = left;
            if( left != NULL )
        left->uplink = up;
            left = up;
        }
        prev = up;
        up = upup;
    }

# ifdef DEBUG
    if( q->root != prev )
    {
/*	fprintf(stderr, " *** bug in splay: n not in q *** " ); */
    abort();
    }
# endif

    n->leftlink = left;
    n->rightlink = right;
    if( left != NULL )
    left->uplink = n;
    if( right != NULL )
    right->uplink = n;
    q->root = n;
    n->uplink = NULL;

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
SPBLK *
sphead( SPTREE* q )
{
    SPBLK * x;

    /* splay version, good amortized bound */
    x = spdeq( &q->root );
    if( x != NULL )
    {
        x->rightlink = q->root;
        x->leftlink = NULL;
        x->uplink = NULL;
        if( q->root != NULL )
        q->root->uplink = x;
    }
    q->root = x;

    /* alternative version, bad amortized bound,
       but faster on the average */

    return( x );

} /* sphead */


/*----------------
 *
 * spdelete() -- Delete node from a tree.
 *
 *	n is deleted from q; the resulting splay tree has been splayed
 *	around its new root, which is the successor of n
 *
 */
void
spdelete( SPBLK* n, SPTREE* q )
{
    SPBLK * x;

    splay( n, q );
    x = spdeq( &q->root->rightlink );
    if( x == NULL )		/* empty right subtree */
    {
        q->root = q->root->leftlink;
        if (q->root) q->root->uplink = NULL;
    }
    else			/* non-empty right subtree */
    {
        x->uplink = NULL;
        x->leftlink = q->root->leftlink;
        x->rightlink = q->root->rightlink;
        if( x->leftlink != NULL )
        x->leftlink->uplink = x;
        if( x->rightlink != NULL )
        x->rightlink->uplink = x;
        q->root = x;
    }

} /* spdelete */

/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrniv/tqueue.h"
#include "coreneuron/nrniv/pool.h"

#if COLLECT_TQueue_STATISTICS
#define STAT(arg) ++arg;
#else
#define STAT(arg) /**/
#endif

//----------------

implementPool(TQItemPool, TQItem)

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

#define SPBLK TQItem
#define leftlink left_
#define rightlink right_
#define uplink parent_
#define cnt cnt_
#define key t_

#include "coreneuron/nrniv/sptree.h"

extern "C" {
//extern double dt;
#define nt_dt nrn_threads->_dt
}

TQItem::TQItem() {
    left_ = 0;
    right_ = 0;
    parent_ = 0;
}

TQItem::~TQItem() {
}

bool TQItem::check() {
#if DOCHECK
#endif
    return true;
}

static void prnt(const TQItem* b, int level) {
    int i;
    for (i=0; i < level; ++i) {
        printf("    ");
    }
    printf("%g %c %d Q=%p D=%p\n", b->t_, b->data_?'x':'o', b->cnt_, (void*)b, (void*)b->data_);
}

TQueue::TQueue(TQItemPool* tp, int mkmut) {
    MUTCONSTRUCT(mkmut)
    tpool_ = tp;
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
#if FAST_LEAST
    if (least_) {
        prnt(least_, 0);
    }
#endif
    spscan(prnt, NULL, sptree_);
    for (TQItem* q = binq_->first(); q; q = binq_->next(q)) {
        prnt(q, 0);
    }
    MUTUNLOCK
}

void TQueue::forall_callback(void(*f)(const TQItem*, int)) {
    MUTLOCK
#if FAST_LEAST
    if (least_) {
        f(least_, 0);
    }
#endif
    spscan(f, NULL, sptree_);
    for (TQItem* q = binq_->first(); q; q = binq_->next(q)) {
        f(q, 0);
    }
    MUTUNLOCK
}

void TQueue::check(const char* mes) {
    (void)mes;
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

void TQueue::spike_stat(double* d) {
#if COLLECT_TQueue_STATISTICS
    d[0] = ninsert;
    d[1] = nmove;
    d[2] = nrem;
//printf("FifoQ spikestat nfenq=%lu nfdeq=%lu nfrem=%lu\n", fifo_->nfenq, fifo_->nfdeq, fifo_->nfrem);
#endif
}

TQItem* TQueue::insert(double tt, void* d) {
    MUTLOCK
    STAT(ninsert);
    TQItem* i = tpool_->alloc();
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
            }else{
                least_ = NULL;
            }
        }else if (q->cnt_ >= 0) {
            binq_->remove(q);
        }else{
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
        }else{
            least_ = NULL;
        }
    }
    MUTUNLOCK
    return q;
}

TQItem* TQueue::find(double tt) {
    TQItem* q;
    MUTLOCK
    // search only in the  splay tree. if this is a bug then fix it.
    STAT(nfind)
    if (tt == least_t_nolock()) {
        q = least();
    }else{
        q = splookup(tt, sptree_);
    }
    MUTUNLOCK
    return(q);
}

BinQ::BinQ() {
    nbin_ = 1000;
    bins_ = new TQItem*[nbin_];
    for (int i=0; i < nbin_; ++i) { bins_[i] = 0; }
    qpt_ = 0;
    tt_ = 0.;
#if COLLECT_TQueue_STATISTICS
    nfenq = nfdeq = nfrem = 0;
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
    int idt = (int)((td - tt_)/nt_dt + 1.e-10);
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

/*
  Previously included as:
  spaux.c:  This code implements the following operations on an event-set
  or priority-queue implemented using splay trees:

  n = sphead( q )		n is the head item in q (not removed).
  spdelete( n, q )		n is removed from q.
  n = spnext( np, q )		n is the successor of np in q.
  n = spprev( np, q )		n is the predecessor of np in q.
  spenqbefore( n, np, q )	n becomes the predecessor of np in q.
  spenqafter( n, np, q )	n becomes the successor of np in q.

  In the above, n and np are pointers to single items (type
  SPBLK *); q is an event-set (type SPTREE *),
  The type definitions for these are taken
  from file sptree.h.  All of these operations rest on basic
  splay tree operations from file sptree.c.

  The basic splay tree algorithms were originally presented in:

  Self Adjusting Binary Trees,
  by D. D. Sleator and R. E. Tarjan,
  Proc. ACM SIGACT Symposium on Theory
  of Computing (Boston, Apr 1983) 235-245.

  The operations in this package supplement the operations from
  file splay.h to provide support for operations typically needed
  on the pending event set in discrete event simulation.  See, for
  example,

  Introduction to Simula 67,
  by Gunther Lamprecht, Vieweg & Sohn, Braucschweig, Wiesbaden, 1981.
  (Chapter 14 contains the relevant discussion.)

  Simula Begin,
  by Graham M. Birtwistle, et al, Studentlitteratur, Lund, 1979.
  (Chapter 9 contains the relevant discussion.)

  Many of the routines in this package use the splay procedure,
  for bottom-up splaying of the queue.  Consequently, item n in
  delete and item np in all operations listed above must be in the
  event-set prior to the call or the results will be
  unpredictable (eg:  chaos will ensue).

  Note that, in all cases, these operations can be replaced with
  the corresponding operations formulated for a conventional
  lexicographically ordered tree.  The versions here all use the
  splay operation to ensure the amortized bounds; this usually
  leads to a very compact formulation of the operations
  themselves, but it may slow the average performance.

  Alternative versions based on simple binary tree operations are
  provided (commented out) for head, next, and prev, since these
  are frequently used to traverse the entire data structure, and
  the cost of traversal is independent of the shape of the
  structure, so the extra time taken by splay in this context is
  wasted.

  This code was written by:
  Douglas W. Jones with assistance from Srinivas R. Sataluri

  Translated to C by David Brower, daveb@rtech.uucp

  Thu Oct  6 12:11:33 PDT 1988 (daveb) Fixed spdeq, which was broken
    handling one-node trees.  I botched the pascal translation of
    a VAR parameter.  Changed interface, so callers must also be
    corrected to pass the node by address rather than value.
  Mon Apr  3 15:18:32 PDT 1989 (daveb)
    Apply fix supplied by Mark Moraes <moraes@csri.toronto.edu> to
    spdelete(), which dropped core when taking out the last element
    in a subtree -- that is, when the right subtree was empty and
    the leftlink was also null, it tried to take out the leftlink's
    uplink anyway.
 */

#include "coreneuron/nrniv/sptree.h"

/*----------------
 *
 * sphead() --	return the "lowest" element in the tree.
 *
 *	returns a reference to the head event in the event-set q,
 *	represented as a splay tree; q->root ends up pointing to the head
 *	event, and the old left branch of q is shortened, as if q had
 *	been splayed about the head element; this is done by dequeueing
 *	the head and then making the resulting queue the right son of
 *	the head returned by spdeq; an alternative is provided which
 *	avoids splaying but just searches for and returns a pointer to
 *	the bottom of the left branch
 */
SPBLK *
sphead( SPTREE* q )
{
    register SPBLK * x;

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
    register SPBLK * x;

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



/*----------------
 *
 * spnext() -- return next higer item in the tree, or NULL.
 *
 *	return the successor of n in q, represented as a splay tree; the
 *	successor becomes the root; two alternate versions are provided,
 *	one which is shorter, but slower, and one which is faster on the
 *	average because it does not do any splaying
 *
 */
SPBLK *
spnext( SPBLK* n, SPTREE* q )
{
    register SPBLK * next;
    register SPBLK * x;

    /* splay version */
    splay( n, q );
    x = spdeq( &n->rightlink );
    if( x != NULL )
    {
        x->leftlink = n;
        n->uplink = x;
        x->rightlink = n->rightlink;
        n->rightlink = NULL;
        if( x->rightlink != NULL )
        x->rightlink->uplink = x;
        q->root = x;
        x->uplink = NULL;
    }
    next = x;

    /* shorter slower version;
       deleting last "if" undoes the amortized bound */

    return( next );

} /* spnext */



/*----------------
 *
 * spprev() -- return previous node in a tree, or NULL.
 *
 *	return the predecessor of n in q, represented as a splay tree;
 *	the predecessor becomes the root; an alternate version is
 *	provided which is faster on the average because it does not do
 *	any splaying
 *
 */
SPBLK *
spprev( SPBLK* n, SPTREE* q )
{
    register SPBLK * prev;
    register SPBLK * x;

    /* splay version;
       note: deleting the last "if" undoes the amortized bound */

    splay( n, q );
    x = n->leftlink;
    if( x != NULL )
    while( x->rightlink != NULL )
        x = x->rightlink;
    prev = x;
    if( x != NULL )
    splay( x, q );

    return( prev );

} /* spprev */



/*----------------
 *
 * spenqbefore() -- insert node before another in a tree.
 *
 *	returns pointer to n.
 *
 *	event n is entered in the splay tree q as the immediate
 *	predecessor of n1; in doing so, n1 becomes the root of the tree
 *	with n as its left son
 *
 */
SPBLK *
spenqbefore( SPBLK* n, SPBLK* n1, SPTREE* q )
{
    splay( n1, q );
    n->key = n1->key;
    n->leftlink = n1->leftlink;
    if( n->leftlink != NULL )
    n->leftlink->uplink = n;
    n->rightlink = NULL;
    n->uplink = n1;
    n1->leftlink = n;

    return( n );

} /* spenqbefore */



/*----------------
 *
 * spenqafter() -- enter n after n1 in tree q.
 *
 *	returns a pointer to n.
 *
 *	event n is entered in the splay tree q as the immediate
 *	successor of n1; in doing so, n1 becomes the root of the tree
 *	with n as its right son
 */
SPBLK *
spenqafter( SPBLK* n, SPBLK* n1, SPTREE* q )
{
    splay( n1, q );
    n->key = n1->key;
    n->rightlink = n1->rightlink;
    if( n->rightlink != NULL )
    n->rightlink->uplink = n;
    n->leftlink = NULL;
    n->uplink = n1;
    n1->rightlink = n;

    return( n );

} /* spenqafter */

/************* End of spaux.c ***************/

/*
 *  Previously included as:
 *  sptree.c:  The following code implements the basic operations on
 *  an event-set or priority-queue implemented using splay trees:
 *
Hines changed to void spinit(SPTREE**) for use with TQueue.
 *  SPTREE *spinit( compare )	Make a new tree
 *  int spempty();		Is tree empty?
 *  SPBLK *spenq( n, q )	Insert n in q after all equal keys.
 *  SPBLK *spdeq( np )		Return first key under *np, removing it.
 *  SPBLK *spenqprior( n, q )	Insert n in q before all equal keys.
 *  void splay( n, q )		n (already in q) becomes the root.
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
    q->lookups = 0;
    q->lkpcmps = 0;
    q->enqs = 0;
    q->enqcmps = 0;
    q->splays = 0;
    q->splayloops = 0;
    q->root = NULL;
}

/*----------------
 *
 * spempty() -- is an event-set represented as a splay tree empty?
 */
int
spempty( SPTREE* q )
{
    return( q == NULL || q->root == NULL );
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
    register SPBLK * left;	/* the rightmost node in the left tree */
    register SPBLK * right;	/* the leftmost node in the right tree */
    register SPBLK * next;	/* the root of the unsplit part */
    register SPBLK * temp;

    double key;
#if STRCMP_DEF
    register int Sct;		/* Strcmp value */
#endif

    q->enqs++;
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

#if BBTQ != 4 && BBTQ != 5
    n->cnt++;
#endif
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
    register SPBLK * deq;		/* one to return */
    register SPBLK * next;       	/* the next thing to deal with */
    register SPBLK * left;      	/* the left child of next */
    register SPBLK * farleft;		/* the left child of left */
    register SPBLK * farfarleft;	/* the left child of farleft */

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
 *  spenqprior() -- insert into tree before other equal keys.
 *
 *  put n in q before all other nodes with the same key; after this is
 *  done, n will be the root of the splay tree representing q, all nodes in
 *  q with keys less than that of n will be in the left subtree, all with
 *  greater or equal keys will be in the right subtree; the tree is split
 *  into these subtrees from the top down, with rotations performed along
 *  the way to shorten the left branch of the right subtree and the right
 *  branch of the left subtree; the logic of spenqprior is exactly the
 *  same as that of spenq except for a substitution of comparison
 *  operators
 */
SPBLK *
spenqprior( SPBLK* n, SPTREE* q )
{

    register SPBLK * left;	/* the rightmost node in the left tree */
    register SPBLK * right;	/* the leftmost node in the right tree */
    register SPBLK * next;	/* the root of unsplit part of tree */
    register SPBLK * temp;
#if STRCMP_DEF
    register int Sct;		/* Strcmp value */
#endif
    double key;

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

        if( STRCMP( next->key, key ) >= 0 )
        goto two;

    one:	/* assert next->key < key */

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
            if( STRCMP( temp->key, key ) >= 0 )
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

    } while( STRCMP( next->key, key ) < 0 );	/* change sides */

    two:	/* assert next->key >= key */

    do	 /* walk to the left in the right tree */
    {
            temp = next->leftlink;
            if( temp == NULL )
        {
                right->leftlink = next;
                next->uplink = right;
                left->rightlink = NULL;
                goto done;	/* job done, entire tree split */
            }
            if( STRCMP( temp->key, key ) < 0 )
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

    } while( STRCMP( next->key, key ) >= 0 );	/* change sides */

        goto one;

    done:	/* split is done, branches of n need reversal */

        temp = n->leftlink;
        n->leftlink = n->rightlink;
        n->rightlink = temp;
    }

    return( n );

} /* spenqprior */

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
    register SPBLK * up;	/* points to the node being dealt with */
    register SPBLK * prev;	/* a descendent of up, already dealt with */
    register SPBLK * upup;	/* the parent of up */
    register SPBLK * upupup;	/* the grandparent of up */
    register SPBLK * left;	/* the top of left subtree being built */
    register SPBLK * right;	/* the top of right subtree being built */

#if BBTQ != 4 && BBTQ != 5
    n->cnt++;	/* bump reference count */
#endif

    left = n->leftlink;
    right = n->rightlink;
    prev = n;
    up = prev->uplink;

    q->splays++;

    while( up != NULL )
    {
    q->splayloops++;

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

/**************** End of sptree.c ********************/

/*
 * Previously included as:
 * spdaveb.c -- daveb's new splay tree functions.
 *
 * The functions in this file provide an interface that is nearly
 * the same as the hash library I swiped from mkmf, allowing
 * replacement of one by the other.  Hey, it worked for me!
 *
 * splookup() -- given a key, find a node in a tree.
 * spinstall() -- install an item in the tree, overwriting existing value.
 * spfhead() -- fast (non-splay) find the first node in a tree.
 * spftail() -- fast (non-splay) find the last node in a tree.
 * spscan() -- forward scan tree from the head.
 * sprscan() -- reverse scan tree from the tail.
 * spfnext() -- non-splaying next.
 * spfprev() -- non-splaying prev.
 * spstats() -- make char string of stats for a tree.
 *
 * Written by David Brower, daveb@rtech.uucp 1/88.
 */

/*----------------
 *
 * splookup() -- given key, find a node in a tree.
 *
 *	Splays the found node to the root.
 */
SPBLK *
splookup( double key, SPTREE* q )
{
    register SPBLK * n;
    register int Sct;
    register int c;

    /* find node in the tree */
    n = q->root;
    c = ++(q->lkpcmps);
    q->lookups++;
//    while( n && (Sct = STRCMP( key, n->key ) ) )
    while( n && (Sct = key!= n->key ) )
    {
    c++;
    n = ( Sct < 0 ) ? n->leftlink : n->rightlink;
    }
    q->lkpcmps = c;

    /* reorganize tree around this node */
    if( n != NULL )
    splay( n, q );

    return( n );
}

/*----------------
 *
 * spfhead() --	return the "lowest" element in the tree.
 *
 *	returns a reference to the head event in the event-set q.
 *	avoids splaying but just searches for and returns a pointer to
 *	the bottom of the left branch.
 */
SPBLK *
spfhead( SPTREE* q )
{
    register SPBLK * x;

    if( NULL != ( x = q->root ) )
    while( x->leftlink != NULL )
        x = x->leftlink;

    return( x );

} /* spfhead */




/*----------------
 *
 * spftail() -- find the last node in a tree.
 *
 *	Fast version does not splay result or intermediate steps.
 */
SPBLK *
spftail( SPTREE* q )
{
    register SPBLK * x;


    if( NULL != ( x = q->root ) )
    while( x->rightlink != NULL )
        x = x->rightlink;

    return( x );

} /* spftail */


/*----------------
 *
 * spscan() -- apply a function to nodes in ascending order.
 *
 *	if n is given, start at that node, otherwise start from
 *	the head.
 */
void
spscan( void (*f)(const TQItem*, int), SPBLK* n, SPTREE* q )
{
    register SPBLK * x;

    for( x = n != NULL ? n : spfhead( q ); x != NULL ; x = spfnext( x ) )
        (*f)( x, 0);
}



/*----------------
 *
 * sprscan() -- apply a function to nodes in descending order.
 *
 *	if n is given, start at that node, otherwise start from
 *	the tail.
 */
void
sprscan( void (*f)(const TQItem*, int), SPBLK* n, SPTREE* q )
{
    register SPBLK *x;

    for( x = n != NULL ? n : spftail( q ); x != NULL ; x = spfprev( x ) )
        (*f)( x, 0 );
}



/*----------------
 *
 * spfnext() -- fast return next higer item in the tree, or NULL.
 *
 *	return the successor of n in q, represented as a splay tree.
 *	This is a fast (on average) version that does not splay.
 */
SPBLK *
spfnext( SPBLK* n )
{
    register SPBLK * next;
    register SPBLK * x;

    /* a long version, avoids splaying for fast average,
     * poor amortized bound
     */

    if( n == NULL )
        return( n );

    x = n->rightlink;
    if( x != NULL )
    {
        while( x->leftlink != NULL )
        x = x->leftlink;
        next = x;
    }
    else	/* x == NULL */
    {
        x = n->uplink;
        next = NULL;
        while( x != NULL )
    {
            if( x->leftlink == n )
        {
                next = x;
                x = NULL;
            }
        else
        {
                n = x;
                x = n->uplink;
            }
        }
    }

    return( next );

} /* spfnext */



/*----------------
 *
 * spfprev() -- return fast previous node in a tree, or NULL.
 *
 *	return the predecessor of n in q, represented as a splay tree.
 *	This is a fast (on average) version that does not splay.
 */
SPBLK *
spfprev( SPBLK* n )
{
    register SPBLK * prev;
    register SPBLK * x;

    /* a long version,
     * avoids splaying for fast average, poor amortized bound
     */

    if( n == NULL )
        return( n );

    x = n->leftlink;
    if( x != NULL )
    {
        while( x->rightlink != NULL )
        x = x->rightlink;
        prev = x;
    }
    else
    {
        x = n->uplink;
        prev = NULL;
        while( x != NULL )
    {
            if( x->rightlink == n )
        {
                prev = x;
                x = NULL;
            }
        else
        {
                n = x;
                x = n->uplink;
            }
        }
    }

    return( prev );

} /* spfprev */



const char *
spstats( SPTREE* q )
{
    static char buf[ 128 ];
    float llen;
    float elen;
    float sloops;

    if( q == NULL )
    return("");

    llen = q->lookups ? (float)q->lkpcmps / q->lookups : 0;
    elen = q->enqs ? (float)q->enqcmps/q->enqs : 0;
    sloops = q->splays ? (float)q->splayloops/q->splays : 0;

    sprintf(buf, "f(%d %4.2f) i(%d %4.2f) s(%d %4.2f)",
    q->lookups, llen, q->enqs, elen, q->splays, sloops );

    return (const char*)buf;
}

/************** End of spdaveb.c ******************/


SelfQueue::SelfQueue(TQItemPool* tp, int mkmut) {
	MUTCONSTRUCT(mkmut)
	tpool_ = tp;
	head_ = NULL;
}
SelfQueue::~SelfQueue() {
	remove_all();
	MUTDESTRUCT
}
TQItem* SelfQueue::insert(void* d) {
	MUTLOCK
	TQItem* q = tpool_->alloc();
	q->left_ = NULL;
	q->right_ = head_;
	if (head_) { head_->left_ = q; }
	head_ = q;
	q->data_ = d;
	MUTUNLOCK
	return q;
}
void* SelfQueue::remove(TQItem* q) {
	MUTLOCK
	if (q->left_) { q->left_->right_ = q->right_; }
	if (q->right_) { q->right_->left_ = q->left_; }
	if (q == head_) { head_ = q->right_; }
	tpool_->hpfree(q);
	MUTUNLOCK
	return q->data_;
}
void SelfQueue::remove_all() {
	MUTLOCK
	for (TQItem* q = first(); q; q = next(q)) {
		tpool_->hpfree(q);
	}
	head_ = NULL;
	MUTUNLOCK
}



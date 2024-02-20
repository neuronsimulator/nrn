/*
** sptree.hpp:  The following type declarations provide the binary tree
**  representation of event-sets or priority queues needed by splay trees
**
**  assumes that data and datb will be provided by the application
**  to hold all application specific information
**
**  assumes that key will be provided by the application, comparable
**  with the compare function applied to the addresses of two keys.
*/

#pragma once

#define STRCMP(a, b) (a - b)

template <typename SPBLK>
struct SPTREE {
    SPBLK* root; /* root node */

    int enqcmps; /* compares in spenq */
};

#define spinit      sptq_spinit
#define spempty     sptq_spempty
#define spenq       sptq_spenq
#define spdeq       sptq_spdeq
#define splay       sptq_splay
#define sphead      sptq_sphead
#define spdelete    sptq_spdelete
#define splookup    sptq_splookup
#define spscan  sptq_spscan
#define spfhead sptq_spfhead
#define spfnext sptq_spfnext

/*  Original file: sptree.cpp
 *
 *  sptree.cpp:  The following code implements the basic operations on
 *  an event-set or priority-queue implemented using splay trees:
 *
Hines changed to void spinit(SPTREE<SPBLK>**) for use with TQueue.
 *  SPTREE *spinit( compare )	Make a new tree
 *  int spempty();		Is tree empty?
 *  SPBLK *spenq( n, q )	Insert n in q after all equal keys.
 *  SPBLK *spdeq( np )		Return first key under *np, removing it.
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


/* USER SUPPLIED! */

/*extern char *emalloc();*/


/*----------------
 *
 * spinit() -- initialize an empty splay tree
 *
 */
template <typename SPBLK>
void spinit(SPTREE<SPBLK>* q) {
    q->enqcmps = 0;
    q->root = nullptr;
}

/*----------------
 *
 * spempty() -- is an event-set represented as a splay tree empty?
 */
template <typename SPBLK>
int spempty(SPTREE<SPBLK>* q) {
    return (q == nullptr || q->root == nullptr);
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
template <typename SPBLK>
SPBLK* spenq(SPBLK* n, SPTREE<SPBLK>* q) {
    SPBLK* left;  /* the rightmost node in the left tree */
    SPBLK* right; /* the leftmost node in the right tree */
    SPBLK* next;  /* the root of the unsplit part */
    SPBLK* temp;

    double key;
    int Sct; /* Strcmp value */

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
template <typename SPBLK>
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

template <typename SPBLK>
void splay(SPBLK* n, SPTREE<SPBLK>* q) {
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


/* Original file: spaux.cpp

  spaux.cpp:  This code implements the following operations on an event-set
  or priority-queue implemented using splay trees:

  n = sphead( q )		n is the head item in q (not removed).
  spdelete( n, q )		n is removed from q.

  In the above, n and np are pointers to single items (type
  SPBLK *); q is an event-set (type SPTREE *),
  The type definitions for these are taken
  from file sptree.h.  All of these operations rest on basic
  splay tree operations from file sptree.cpp.

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
template <typename SPBLK>
SPBLK* sphead(SPTREE<SPBLK>* q) {
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
template <typename SPBLK>
void spdelete(SPBLK* n, SPTREE<SPBLK>* q) {
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


/* Original file: spdaveb.cpp
 *
 * spdaveb.cpp -- daveb's new splay tree functions.
 *
 * The functions in this file provide an interface that is nearly
 * the same as the hash library I swiped from mkmf, allowing
 * replacement of one by the other.  Hey, it worked for me!
 *
 * splookup() -- given a key, find a node in a tree.
 * spfhead() -- fast (non-splay) find the first node in a tree.
 * spscan() -- forward scan tree from the head.
 * spfnext() -- non-splaying next.
 *
 * Written by David Brower, daveb@rtech.uucp 1/88.
 */


/*----------------
 *
 * splookup() -- given key, find a node in a tree.
 *
 *	Splays the found node to the root.
 */
template <typename SPBLK>
SPBLK* splookup(double key, SPTREE<SPBLK>* q) {
    SPBLK* n;
    int Sct;

    /* find node in the tree */
    n = q->root;
    //    while( n && (Sct = STRCMP( key, n->key ) ) )
    while (n && (Sct = key != n->key)) {
        n = (Sct < 0) ? n->leftlink : n->rightlink;
    }

    /* reorganize tree around this node */
    if (n != nullptr)
        splay(n, q);

    return (n);
}


/*----------------
 *
 * spfhead() --	return the "lowest" element in the tree.
 *
 *	returns a reference to the head event in the event-set q.
 *	avoids splaying but just searches for and returns a pointer to
 *	the bottom of the left branch.
 */
template <typename SPBLK>
SPBLK* spfhead(SPTREE<SPBLK>* q) {
    SPBLK* x;

    if (nullptr != (x = q->root))
        while (x->leftlink != nullptr)
            x = x->leftlink;

    return (x);

} /* spfhead */


/*----------------
 *
 * spscan() -- apply a function to nodes in ascending order.
 *
 *	if n is given, start at that node, otherwise start from
 *	the head.
 */
template <typename SPBLK>
void spscan(void (*f)(const SPBLK*, int), SPBLK* n, SPTREE<SPBLK>* q) {
    SPBLK* x;

    for (x = n != nullptr ? n : spfhead<SPBLK>(q); x != nullptr; x = spfnext(x))
        (*f)(x, 0);
}

/*----------------
 *
 * spfnext() -- fast return next higer item in the tree, or nullptr.
 *
 *	return the successor of n in q, represented as a splay tree.
 *	This is a fast (on average) version that does not splay.
 */
template <typename SPBLK>
SPBLK* spfnext(SPBLK* n) {
    SPBLK* next;
    SPBLK* x;

    /* a long version, avoids splaying for fast average,
     * poor amortized bound
     */

    if (n == nullptr)
        return (n);

    x = n->rightlink;
    if (x != nullptr) {
        while (x->leftlink != nullptr)
            x = x->leftlink;
        next = x;
    } else /* x == nullptr */
    {
        x = n->uplink;
        next = nullptr;
        while (x != nullptr) {
            if (x->leftlink == n) {
                next = x;
                x = nullptr;
            } else {
                n = x;
                x = n->uplink;
            }
        }
    }

    return (next);

} /* spfnext */

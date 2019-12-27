/*
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

# include	"sptree.h"


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
    
# if 0
    x = q->root;
    while( x->leftlink != NULL )
	x = x->leftlink;
# endif
    
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
    
# if 0
    splay( n, q );
    x = n->rightlink;
    if( x != NULL )
	while( x->leftlink != NULL )
	    x = x->leftlink;
    next = x;
    if( x != NULL )
	splay( x, q );
# endif
    
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



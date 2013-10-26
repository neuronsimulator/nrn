/*
 *
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

# include "sptree.h"

/* USER SUPPLIED! */

/*extern char *emalloc();*/


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
    register int Sct;		/* Strcmp value */

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
    register int Sct;		/* Strcmp value */
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


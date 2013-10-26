/*
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


# include "sptree.h"

/* USER SUPPLIED! */

/*extern char *emalloc();*/


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


#if 0

/*----------------
 *
 * spinstall() -- install an entry in a tree, overwriting any existing node.
 *
 *	If the node already exists, replace its contents.
 *	If it does not exist, then allocate a new node and fill it in.
 */

SPBLK *
spinstall( key, data, datb, q )

register char * key;
register char * data;
register char * datb;
register SPTREE *q;

{
    register SPBLK *n;

    if( NULL == ( n = splookup( key, q ) ) )
    {
	n = (SPBLK *) emalloc( sizeof( *n ) );
	n->key = key;
	n->leftlink = NULL;
	n->rightlink = NULL;
	n->uplink = NULL;
#if BBTQ != 4 && BBTQ != 5
	n->cnt = 0;
#endif
	spenq( n, q );
    }

    n->data = data;
    n->datb = datb;

    return( n );
}
#endif



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


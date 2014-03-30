#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/nmodl/list.c,v 4.2 1998/01/22 18:50:32 hines Exp */
/*
list.c,v
 * Revision 4.2  1998/01/22  18:50:32  hines
 * allow stralloc with null string (creates empty string)
 *
 * Revision 4.1  1997/08/30  20:45:25  hines
 * cvs problem with branches. Latest nmodl stuff should now be a top level
 *
 * Revision 4.0.1.1  1997/08/08  17:23:50  hines
 * nocmodl version 4.0.1
 *
 * Revision 4.0  1997/08/08  17:06:16  hines
 * proper nocmodl version number
 *
 * Revision 1.1.1.1  1994/10/12  17:21:35  hines
 * NEURON 3.0 distribution
 *
 * Revision 9.159  93/02/11  16:55:37  hines
 * minor mods for NeXT
 * 
 * Revision 9.78  90/12/10  16:56:39  hines
 * TABLE allowed in FUNCTION and PROCEDURE
 * 
 * Revision 9.76  90/12/07  09:27:14  hines
 * new list structure that uses unions instead of void *element
 * 
 * Revision 8.1  89/09/29  16:26:03  mlh
 * ifdef for VMS and SYSV and some fixing of assert
 * 
 * Revision 8.0  89/09/22  17:26:18  nfh
 * Freezing
 * 
 * Revision 7.1  89/09/05  08:07:12  mlh
 * lappenditem() for use with lists of items which point to items
 * ITM(q) analogous to SYM(q)
 * 
 * Revision 7.0  89/08/30  13:31:47  nfh
 * Rev 7 is now Experimental; Rev 6 is Testing
 * 
 * Revision 6.0  89/08/14  16:26:30  nfh
 * Rev 6.0 is latest of 4.x; now the Experimental version
 * 
 * Revision 4.1  89/08/07  15:34:30  mlh
 * freelist now takes pointer to list pointer and 0's the list pointer.
 * Not doing this is a bug for multiple sens blocks, etc.
 * 
 * Revision 4.0  89/07/24  17:02:57  nfh
 * Freezing rev 3.  Rev 4 is now Experimental
 * 
 * Revision 3.1  89/07/07  16:54:16  mlh
 * FIRST LAST START in independent SWEEP higher order derivatives
 * 
 * Revision 1.1  89/07/06  14:49:26  mlh
 * Initial revision
 * 
*/

/* The following routines support the concept of a list.
That is, one can insert at the head of a list or append to the tail of a
list with linsert<type>() and lappend<type>().

In addition, one can insert an item before a known item and it will be
placed in the proper list.

Items point to strings, symbols, etc. Note that more than one item in the
same or several lists can point to the same string, symbol.

Finally, knowing an item, one can determine the preceding and following
items with next() and prev().

Deletion, replacement and moving blocks of items is also supported.
*/

/* Implementation
  The list is a doubly linked list. A special item with element 0 is
  always at the tail of the list and is denoted as the List pointer itself.
  list->next point to the first item in the list and
  list->prev points to the last item in the list.
	i.e. the list is circular
  Note that in an empty list next and prev points to itself.

It is intended that this implementation be hidden from the user via the
following function calls.
*/

#include <stdlib.h>
#include	"modl.h"
#include	"parse1.h"

static Item *newitem()
{
	return (Item *)emalloc(sizeof(Item));
}

List *newlist()
{
	Item *i;
	i = newitem();
	i->prev = i;
	i->next = i;
	i->element.lst = (List *)0;
	i->itemtype = 0;
	return (List *)i;
}

void freelist(plist)	/*free the list but not the elements*/
	List **plist;
{
	Item *i1, *i2;
	if (!(*plist)) {
		return;
	}
	for (i1 = (*plist)->next; i1 != *plist; i1 = i2) {
		i2 = i1->next;
		Free(i1);
	}
	Free(*plist);
	*plist = (List *)0;
}

static Item *linkitem(item)
	Item *item;
{
	Item *i;

	i = newitem();
	i->prev = item->prev;
	i->next = item;
	item->prev = i;
	i->prev->next = i;
	return i;
}

#if 0	 /*currently unused*/
Item *next(item)
	Item *item;
{
	assert(item->next->element.lst); /* never return the list item */
	return item->next;
}

Item *prev(item)
	Item *item;
{
	assert(item->prev->element.lst); /* never return the list item */
	return item->prev;
}
#endif

Item *insertstr(item, str) /* insert a copy of the string before item */
/* a copy is made because strings are often assembled into a reusable buffer*/
	Item *item;
	char *str;
{
	Item *i;

	i = linkitem(item);
	i->element.str = stralloc(str, (char *)0);
	i->itemtype = STRING;
	return i;
}

Item *insertitem(item, itm) /* insert a item pointer before item */
	Item *item, *itm;
{
	Item *i;

	i = linkitem(item);
	i->element.itm = itm;
	i->itemtype = ITEM;
	return i;
}

Item *insertlist(item, lst) /* insert a item pointer before item */
	Item *item;
	List *lst;
{
	Item *i;

	i = linkitem(item);
	i->element.lst = lst;
	i->itemtype = LIST;
	return i;
}
	
Item *insertsym(item, sym) /* insert a symbol before item */
/* a copy is not made because we need the same symbol in different lists */
	Item *item;
	Symbol *sym;
{
	Item *i;

	i = linkitem(item);
	i->element.sym = sym;
	i->itemtype = SYMBOL;
	return i;
}
	
Item *linsertstr(list, str)
	List *list;
	char *str;
{
	return insertstr(list->next, str);
}

Item *lappendstr(list, str)
	List *list;
	char *str;
{
	return insertstr(list, str);
}

Item *linsertsym(list, sym)
	List *list;
	Symbol *sym;
{
	return insertsym(list->next, sym);
}

Item *lappendsym(list, sym)
	List *list;
	Symbol *sym;
{
	return insertsym(list, sym);
}

Item *lappenditem(list, item)
	List *list;
	Item *item;
{
	return insertitem(list, item);
}

Item *lappendlst(list, lst)
	List *list, *lst;
{
	return insertlist(list, lst);
}

void delete(item)
	Item *item;
{
	assert(item->itemtype); /* can't delete list */
	item->next->prev = item->prev;
	item->prev->next = item->next;
	Free(item);
}

char *emalloc(n) unsigned n; { /* check return from malloc */
	char *p;
	p = malloc(n);
	if (p == (char *)0) {
		diag("out of memory", (char *)0);
	}
	return p;
}

char *stralloc(buf, rel) char *buf,*rel; {
	/* allocate space, copy buf, and free rel */
	char *s;
	if (buf) {
		s = emalloc((unsigned)(strlen(buf) + 1));
		Strcpy(s, buf);
	}else{
		s = emalloc(1);
		s[0] = '\0';
	}
	if (rel) {
		Free(rel);
	}
	return s;
}

void deltokens(q1, q2) /* delete tokens from q1 to q2 */
	Item *q1, *q2;
{
	/* It is a serious error if q2 precedes q1 */
	Item *q;
	for (q = q1; q != q2;) {
		q = q->next;
		delete(q->prev);
	}
	delete(q2);
		
}

void move(q1, q2, q3)	/* move q1 to q2 and insert before q3*/
	Item *q1, *q2, *q3;
{
	/* it is a serious error if q2 precedes q1 */
	assert(q1 && q2);
	assert(q1->itemtype && q2->itemtype);
	q1->prev->next = q2->next; /* remove from first list */
	q2->next->prev = q1->prev;

	q1->prev = q3->prev;
	q3->prev->next = q1;
	q3->prev = q2;
	q2->next = q3;
}

void movelist(q1, q2, s)	/* move q1 to q2 from old list to end of list s*/
	Item *q1, *q2;
	List *s;
{
	move(q1, q2, s);
}

void replacstr(q, s)
	Item *q;
	char *s;
{
	q->itemtype = STRING;
	q->element.str = stralloc(s, (char *)0);

}

Item *putintoken(s, type)
	char *s;
	short type;
{	/* make sure a symbol exists for s and
	append to intoken list */
	Symbol *sym;

	if (s == (char *)0)
		diag("internal error"," in putintoken");
	switch (type) {
		
	case STRING:
	case REAL:
	case INTEGER:
		return insertstr(intoken, s);

	default:
		if ((sym = lookup(s)) == SYM0) {
			sym = install(s, type);
		}
		break;
	}
	return insertsym(intoken, sym);
}



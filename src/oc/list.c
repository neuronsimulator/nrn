#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/list.c,v 1.1.1.1 1994/10/12 17:22:11 hines Exp */
/*
list.c,v
 * Revision 1.1.1.1  1994/10/12  17:22:11  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.43  93/04/20  08:39:02  hines
 * lists can have void* elements (use insertvoid lappendvoid VOIDITM(q) )
 * 
 * Revision 1.3  92/10/29  09:20:13  hines
 * some errors in freeing objects fixed and replace usage of getarg for
 * non numbers.
 * 
 * Revision 1.2  92/10/27  12:08:14  hines
 * list.c hoclist.h moved from nrnoc to oc
 * all templates maintain a list of their objects
 * 
 * Revision 1.1  92/10/27  11:40:21  hines
 * Initial revision
 * 
 * Revision 2.75  92/08/07  16:11:58  hines
 * sections as objects. nsection and section gone. now list of sections
 * in nmodl list style in its place. vectorization no longer optional
 * probably many bugs at this point but all examples run correctly.
 * Need tests with complete code coverage.
 * 
 * Revision 1.1  92/08/06  14:38:19  hines
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

#define HOC_L_LIST 1

#include	"hoclist.h"
#include	"hocdec.h"
#include	"parse.h"

#define Free free

static Item *
newitem()
{
	Item* q;
	q = (Item *)emalloc(sizeof(Item));
	return q;
}

List *
newlist()
{
	Item *i;
	i = newitem();
	i->prev = i;
	i->next = i;
	i->element.lst = (List *)0;
	i->itemtype = 0;
	return (List *)i;
}

freelist(plist)	/*free the list but not the elements*/
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

static Item *
linkitem(item)
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

Item *
insertstr(item, str) /* insert a copy of the string before item */
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

Item *
insertitem(item, itm) /* insert a item pointer before item */
	Item *item, *itm;
{
	Item *i;

	i = linkitem(item);
	i->element.itm = itm;
	i->itemtype = ITEM;
	return i;
}

Item *
insertlist(item, lst) /* insert a item pointer before item */
	Item *item;
	List *lst;
{
	Item *i;

	i = linkitem(item);
	i->element.lst = lst;
	i->itemtype = LIST;
	return i;
}
	
Item *
insertsym(item, sym) /* insert a symbol before item */
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

Item *
insertsec(item, sec) /* insert a section before item */
/* a copy is not made because we need the same symbol in different lists */
	Item *item;
	struct Section *sec;
{
	Item *i;

	i = linkitem(item);
	i->element.sec = sec;
	i->itemtype = SECTION;
	return i;
}
	
Item *
insertobj(item, obj) /* insert a object before item */
/* a copy is not made because we need the same symbol in different lists */
	Item *item;
	struct Object *obj;
{
	Item *i;

	i = linkitem(item);
	i->element.obj = obj;
	i->itemtype = OBJECTVAR;
	return i;
}
	
Item *
insertvoid(item, obj) /* insert a void pointer before item */
/* a copy is not made because we need the same symbol in different lists */
	Item *item;
	void *obj;
{
	Item *i;

	i = linkitem(item);
	i->element.vd = obj;
	i->itemtype = VOIDPOINTER;
	return i;
}
	
Item *
linsertstr(list, str)
	List *list;
	char *str;
{
	return insertstr(list->next, str);
}

Item *
lappendstr(list, str)
	List *list;
	char *str;
{
	return insertstr(list, str);
}

Item *
linsertsym(list, sym)
	List *list;
	Symbol *sym;
{
	return insertsym(list->next, sym);
}

Item *
lappendsym(list, sym)
	List *list;
	Symbol *sym;
{
	return insertsym(list, sym);
}

Item *
lappenditem(list, item)
	List *list;
	Item *item;
{
	return insertitem(list, item);
}

Item *
lappendlst(list, lst)
	List *list, *lst;
{
	return insertlist(list, lst);
}

Item *
lappendsec(list, sec)
	List *list;
	struct Section* sec;
{
	return insertsec(list, sec);
}

Item *
lappendobj(list, obj)
	List *list;
	struct Object* obj;
{
	return insertobj(list, obj);
}

Item *
lappendvoid(list, obj)
	List *list;
	void* obj;
{
	return insertvoid(list, obj);
}

delete(item)
	Item *item;
{
	assert(item->itemtype); /* can't delete list */
	item->next->prev = item->prev;
	item->prev->next = item->next;
	Free(item);
}

char *stralloc(buf, rel) char *buf,*rel; {
	/* allocate space, copy buf, and free rel */
	char *s;
	s = emalloc((unsigned)(strlen(buf) + 1));
	Strcpy(s, buf);
	if (rel) {
		Free(rel);
	}
	return s;
}

delitems(q1, q2) /* delete tokens from q1 to q2 */
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

move(q1, q2, q3)	/* move q1 to q2 and insert before q3*/
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

movelist(q1, q2, s)	/* move q1 to q2 from old list to end of list s*/
	Item *q1, *q2;
	List *s;
{
	move(q1, q2, s);
}

replacstr(q, s)
	Item *q;
	char *s;
{
	q->itemtype = STRING;
	q->element.str = stralloc(s, (char *)0);

}

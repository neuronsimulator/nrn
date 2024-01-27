#include <../../nmodlconf.h>

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
#include "modl.h"
#include "parse1.hpp"

static Item* newitem() {
    return (Item*) emalloc(sizeof(Item));
}

List* newlist() {
    Item* i;
    i = newitem();
    i->prev = i;
    i->next = i;
    i->element.lst = (List*) 0;
    i->itemtype = 0;
    return (List*) i;
}

void freelist(List** plist) /*free the list but not the elements*/
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
    *plist = (List*) 0;
}

static Item* linkitem(Item* item) {
    Item* i;

    i = newitem();
    i->prev = item->prev;
    i->next = item;
    item->prev = i;
    i->prev->next = i;
    return i;
}

#if 0 /*currently unused*/
Item *next(	Item *item)
{
	assert(item->next->element.lst); /* never return the list item */
	return item->next;
}

Item *prev(Item *item)
{
	assert(item->prev->element.lst); /* never return the list item */
	return item->prev;
}
#endif

Item* insertstr(Item* item, const char* str) /* insert a copy of the string before item */
/* a copy is made because strings are often assembled into a reusable buffer*/
{
    Item* i;

    i = linkitem(item);
    i->element.str = stralloc(str, (char*) 0);
    i->itemtype = STRING;
    return i;
}

Item* insertitem(Item* item, Item* itm) /* insert a item pointer before item */
{
    Item* i;

    i = linkitem(item);
    i->element.itm = itm;
    i->itemtype = ITEM;
    return i;
}

Item* insertlist(Item* item, List* lst) /* insert a item pointer before item */
{
    Item* i;

    i = linkitem(item);
    i->element.lst = lst;
    i->itemtype = LIST;
    return i;
}

Item* insertsym(Item* item, Symbol* sym) /* insert a symbol before item */
/* a copy is not made because we need the same symbol in different lists */
{
    Item* i;

    i = linkitem(item);
    i->element.sym = sym;
    i->itemtype = SYMBOL;
    return i;
}

Item* linsertstr(List* list, const char* str) {
    return insertstr(list->next, str);
}

Item* lappendstr(List* list, const char* str) {
    return insertstr(list, str);
}

Item* linsertsym(List* list, Symbol* sym) {
    return insertsym(list->next, sym);
}

Item* lappendsym(List* list, Symbol* sym) {
    return insertsym(list, sym);
}

Item* lappenditem(List* list, Item* item) {
    return insertitem(list, item);
}

Item* lappendlst(List* list, List* lst) {
    return insertlist(list, lst);
}

void remove(Item* item) {
    assert(item->itemtype); /* can't delete list */
    item->next->prev = item->prev;
    item->prev->next = item->next;
    Free(item);
}

char* emalloc(unsigned n) { /* check return from malloc */
    char* p;
    p = static_cast<char*>(malloc(n));
    if (p == (char*) 0) {
        diag("out of memory", (char*) 0);
    }
    return p;
}

char* stralloc(const char* buf, char* rel) {
    /* allocate space, copy buf, and free rel */
    char* s;
    if (buf) {
        s = static_cast<char*>(emalloc((unsigned) (strlen(buf) + 1)));
        Strcpy(s, buf);
    } else {
        s = static_cast<char*>(emalloc(1));
        s[0] = '\0';
    }
    if (rel) {
        Free(rel);
    }
    return s;
}

void deltokens(Item* q1, Item* q2) /* delete tokens from q1 to q2 */
{
    /* It is a serious error if q2 precedes q1 */
    Item* q;
    for (q = q1; q != q2;) {
        q = q->next;
        remove(q->prev);
    }
    remove(q2);
}

void move(Item* q1, Item* q2, Item* q3) /* move q1 to q2 and insert before q3*/
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

void movelist(Item* q1, Item* q2, List* s) /* move q1 to q2 from old list to end of list s*/
{
    move(q1, q2, s);
}

void replacstr(Item* q, const char* s) {
    q->itemtype = STRING;
    q->element.str = stralloc(s, (char*) 0);
}

Item* putintoken(const char* s, short type) { /* make sure a symbol exists for s and
                                        append to intoken list */
    Symbol* sym;

    if (s == (char*) 0)
        diag("internal error", " in putintoken");
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

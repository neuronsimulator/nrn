#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/modlunit/symbol.c,v 1.1.1.1 1994/10/12 17:22:50 hines Exp */

#include "model.h"
#include "parse1.hpp"
#include "symbol.h"

List* symlist[128]; /* symbol table: linked list
            first char gives which list to use,but*/

List* symlistlist; /* for a stack of local symbol lists */

void symbol_init() {
    int i;
    symlistlist = newlist();
    for (i = 0; i < 128; i++) { /* more than we need */
        symlist[i] = newlist();
    }
}

Symbol* lookup(const char* s) /* find s in symbol table */
{
    Item* sp;

    ITERATE(sp, symlist[s[0]]) {
        if (strcmp(SYM(sp)->name, s) == 0) {
            return SYM(sp);
        }
    }
    return SYM0; /* 0 ==> not found */
}

Symbol* checklocal(Symbol* sym) {
    Item* sp;
    List* sl;
    char* s;

    s = sym->name;
    /* look in local lists */
    ITERATE(sl, symlistlist)
    ITERATE(sp, (List*) sl->element) {
        if (strcmp(SYM(sp)->name, s) == 0) {
            return SYM(sp);
        }
    }
    return sym;
}

Symbol* install(const char* s, int t) /* install s in the list symbol table with type t*/
{
    Symbol* sp;
    List* sl;

    if (t == STRING) {
        sl = symlist[0];
    } else if (t == -1) { /*install on top local list see below*/
        t = NAME;
        assert(symlistlist->next != symlistlist);
        sl = (List*) (symlistlist->next->element);
    } else {
        sl = symlist[s[0]];
    }
    sp = (Symbol*) emalloc(sizeof(Symbol));
    sp->name = stralloc(s, (char*) 0);
    sp->type = t;
    sp->subtype = 0;
    sp->info = ITEM0;
    sp->u.str = (char*) 0;
    sp->used = 0;
    sp->usage = 0;
    sp->araydim = 0;
    sp->discdim = 0;
    Linsertsym(sl, sp); /*insert at head of list*/
    return sp;
}

void pushlocal(Item* q1, Item* qdim) {
    Item* q;
    q = linsertsym(symlistlist, SYM0); /*the type is irrelevant*/
    q->element = (void*) newlist();
    if (q1) {
        install_local(q1, qdim);
    }
}

void poplocal() {
    List* sl;
    Item *i, *j;

    assert(symlistlist->next != symlistlist);
    sl = (List*) symlistlist->next->element;
    for (i = sl->next; i != sl; i = j) {
        j = i->next;
        remove(i);
    }
    remove(symlistlist->next);
}

void install_local(Item* q, Item* qdim) {
    Symbol* s;

    s = install(SYM(q)->name, -1);
    s->subtype = LOCL;
    if (qdim) {
        decdim(s, qdim);
    }
}

/* symbol.c,v
 * Revision 1.1.1.1  1994/10/12  17:22:50  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.5  91/01/25  09:31:37  hines
 * botched last fix
 *
 * Revision 1.4  91/01/24  15:25:19  hines
 * translation error when last token of LOCAL statement was the first token
 * after the LOCAL statement. Fixed by changing symbols at the parser insteadof the lexical
 * analyser.
 *
 * Revision 1.3  90/12/12  11:33:12  hines
 * LOCAL vectors allowed. Some more NEURON syntax added
 *
 * Revision 1.2  90/11/13  16:10:23  hines
 * *** empty log message ***
 *
 * Revision 1.1  90/07/04  09:21:27  hines
 * Initial revision
 *  */

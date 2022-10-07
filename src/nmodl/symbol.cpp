#include <../../nmodlconf.h>

#include "modl.h"
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
    ITERATE(sp, LST(sl)) {
        if (strcmp(SYM(sp)->name + 2, s) == 0) { /*get past _l*/
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
        sl = LST(symlistlist->next);
    } else {
        sl = symlist[s[0]];
    }
    sp = (Symbol*) emalloc(sizeof(Symbol));
    sp->name = stralloc(s, (char*) 0);
    sp->type = t;
    sp->subtype = 0;
    sp->nrntype = 0;
    sp->assigned_to_ = 0;
    sp->no_threadargs = 0;
    sp->slist_info_ = (int*) 0;
    sp->u.str = (char*) 0;
    sp->used = 0;
    sp->usage = 0;
    sp->araydim = 0;
    sp->discdim = 0;
    Linsertsym(sl, sp); /*insert at head of list*/
    return sp;
}

void pushlocal() {
    Item* q;
    q = linsertsym(symlistlist, SYM0); /*the type is irrelevant*/
    LST(q) = newlist();
}

void poplocal() /* a lot of storage leakage here for symbols we are guaranteed
    not to need */
{
    List* sl;
    Item *i, *j;

    assert(symlistlist->next != symlistlist);
    sl = LST(symlistlist->next);
    for (i = sl->next; i != sl; i = j) {
        j = i->next;
        remove(i);
    }
    remove(symlistlist->next);
}

Symbol* copylocal(Symbol* s) {
    if (s->name[0] == '_') {
        Sprintf(buf, "%s", s->name);
    } else {
        Sprintf(buf, "_l%s", s->name);
    }
    return install(buf, -1);
}

#include <../../nmodlconf.h>

#include <stdlib.h>
#include "modl.h"
#include "parse1.hpp"
#include "symbol.h"

void disc_var_seen(Item* q1, Item* q2, Item* q3, int array) /*NAME '@' NUMBER --- array flag*/
{
    Symbol* s;
    int num;

    num = atoi(STR(q3));
    s = SYM(q1);
    if (num < 1) {
        diag("Discrete variable must have @index >= 1", (char*) 0);
    }
    num--;
    if (!(s->subtype & STAT)) {
        diag(s->name, "must be a STATE for use as discrete variable");
    }
    if (array && !(s->subtype & ARRAY)) {
        diag(s->name, "must be a scalar discrete variable");
    }
    if (!array && (s->subtype & ARRAY)) {
        diag(s->name, "must be an array discrete variable");
    }
    if (s->discdim <= num) {
        s->discdim = num + 1;
    }
    Sprintf(buf, "__%s", s->name);
    replacstr(q1, buf);
    remove(q2);
    Sprintf(buf, "[%d]", num);
    replacstr(q3, buf);
}

void massagediscblk(Item* q1, Item* q2, Item* q3, Item* q4) /*DISCRETE NAME stmtlist '}'*/
{
    int i;
    Symbol* s;
    Item* qs;

    replacstr(q1, "void");
    Insertstr(q3, "()\n{\n");
    Insertstr(q4, "}\n");
    SYM(q2)->subtype |= DISCF;
    SYMITER(NAME) if (s->subtype & STAT && s->used && s->discdim) {
        if (s->subtype & ARRAY) {
            Sprintf(buf,
                    "{int _i, _j; for (_j=%d; _j >=0; _j--) {\n\
for (_i=%d; _i>0; _i--) __%s[_i][_j] = __%s[_i-1][_j];\n\
 __%s[0][_j] = %s[_j];\n\
}}\n",
                    s->araydim - 1,
                    s->discdim - 1,
                    s->name,
                    s->name,
                    s->name,
                    s->name);
        } else {
            Sprintf(buf,
                    "{int _i; for (_i=%d; _i>0; _i--) __%s[_i] = __%s[_i-1];\n\
 __%s[0] = %s;\n}\n",
                    s->discdim - 1,
                    s->name,
                    s->name,
                    s->name,
                    s->name);
        }
        Insertstr(q3, buf);
        s->used = 0;
    }
    /*initialization and declaration done elsewhere*/
    movelist(q1, q4, procfunc);
}

void init_disc_vars() {
    int i;
    Item* qs;
    Symbol* s;
    SYMITER(NAME) if (s->subtype & STAT && s->discdim) {
        if (s->subtype & ARRAY) {
            Sprintf(buf,
                    "{int _i, _j; for (_j=%d; _j >=0; _j--) {\n\
for (_i=%d; _i>=0; _i--) __%s[_i][_j] = %s0;}}\n",
                    s->araydim - 1,
                    s->discdim - 1,
                    s->name,
                    s->name);
            Linsertstr(initfunc, buf);
            Sprintf(buf, "static double __%s[%d][%d];\n", s->name, s->discdim, s->araydim);
            Linsertstr(procfunc, buf);
        } else {
            Sprintf(buf,
                    "{int _i; for (_i=%d; _i>=0; _i--) __%s[_i] = %s0;}\n",
                    s->discdim - 1,
                    s->name,
                    s->name);
            Linsertstr(initfunc, buf);
            Sprintf(buf, "static double __%s[%d];\n", s->name, s->discdim);
            Linsertstr(procfunc, buf);
        }
    }
}

#include <../../nmodlconf.h>
#include "model.h"
#include "units.h"
#include "parse1.hpp"

#define IONCUR  0
#define IONEREV 1
#define IONIN   2
#define IONOUT  3

static int point_process = 0;
static List *current, *concen, *potential;
static void unit_chk(const char*, const char*);
static int iontype(char*, char*);

int breakpoint_local_seen_;
int conductance_seen_;

void nrn_unit_init() {
    current = newlist();
    concen = newlist();
    potential = newlist();
}

#if NRNUNIT
void nrn_unit_chk() {
    Item* q;

    unit_chk("v", "millivolt");
    unit_chk("t", "ms");
    unit_chk("dt", "ms");
    unit_chk("celsius", "degC");
    unit_chk("diam", "micron");
    unit_chk("area", "micron2");

    if (breakpoint_local_seen_ == 0 || conductance_seen_ == 0) {
        ITERATE(q, current) {
            if (point_process) {
                unit_chk(SYM(q)->name, "nanoamp");
            } else {
                unit_chk(SYM(q)->name, "milliamp/cm2");
            }
        }
    }
    ITERATE(q, concen) {
        unit_chk(SYM(q)->name, "milli/liter");
    }
    ITERATE(q, potential) {
        unit_chk(SYM(q)->name, "millivolt");
    }
}

static void unit_chk(const char* name, const char* unit) {
    Symbol* s;

    s = lookup(name);
    if (s) {
        Unit_push(decode_units(s));
        Unit_push(unit);
        if (!unit_cmp_exact()) {
            Sprintf(
                buf, "%s must have the units, %s, instead of %s.\n", name, unit, decode_units(s));
            diag(buf, (char*) 0);
        }
        unit_pop();
        unit_pop();
    }
}

void nrn_list(Item* qtype, Item* qlist) {
    List** plist;
    Item* q;

    switch (SYM(qtype)->type) {
    case NONSPECIFIC:
    case ELECTRODE_CURRENT:
        plist = &current;
        break;
    case SUFFIX:
        plist = (List**) 0;
        if (strcmp(SYM(qtype)->name, "POINT_PROCESS") == 0) {
            point_process = 1;
        }
        if (strcmp(SYM(qtype)->name, "ARTIFICIAL_CELL") == 0) {
            point_process = 1;
        }
        break;
    case RANDOM:
        plist = (List**) 0;
        ITERATE(q, qlist) {
            declare(RANGEOBJ, q, nullptr);
        }
        break;
    default:
        plist = (List**) 0;
        break;
    }
    if (plist && qlist) {
        ITERATE(q, qlist) {
            Lappendsym(*plist, SYM(q));
        }
    }
}

void nrn_use(Item* qion, Item* qreadlist, Item* qwritelist) {
    int i;
    List* l;
    Item* q;
    Symbol* ion;

    ion = SYM(qion);
    for (i = 0; i < 2; i++) {
        if (i == 0) {
            l = (List*) qreadlist;
        } else {
            l = (List*) qwritelist;
        }
        if (l)
            ITERATE(q, l) {
                switch (iontype(SYM(q)->name, ion->name)) {
                case IONCUR:
                    Lappendsym(current, SYM(q));
                    break;
                case IONEREV:
                    Lappendsym(potential, SYM(q));
                    break;
                case IONIN:
                case IONOUT:
                    Lappendsym(concen, SYM(q));
                    break;
                }
            }
    }
}

static int iontype(char* s1, char* s2) /* returns index of variable in ion mechanism */
{
    Sprintf(buf, "i%s", s2);
    if (strcmp(buf, s1) == 0) {
        return IONCUR;
    }
    Sprintf(buf, "e%s", s2);
    if (strcmp(buf, s1) == 0) {
        return IONEREV;
    }
    Sprintf(buf, "%si", s2);
    if (strcmp(buf, s1) == 0) {
        return IONIN;
    }
    Sprintf(buf, "%so", s2);
    if (strcmp(buf, s1) == 0) {
        return IONOUT;
    }
    Sprintf(buf, "%s is not a valid ionic variable for %s", s1, s2);
    diag(buf, (char*) 0);
    return -1;
}

#endif /*NRNUNIT*/

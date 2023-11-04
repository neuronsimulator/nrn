#include <../../nrnconf.h>
#define HOC_L_LIST 1
#include "section.h"
#include "neuron.h"
#include "nrnpy.h"
#include "parse.hpp"
#include "hocparse.h"
#include "code.h"
#include "hoc_membf.h"

/* needs trailing '}' */
#define ITERATE_REMOVE(q1, q2, lst)                \
    for (q1 = (lst)->next; q1 != (lst); q1 = q2) { \
        q2 = q1->next;                             \
        if (q1->element.sec->prop == NULL) {       \
            hoc_l_delete(q1);                      \
            continue;                              \
        }

extern int hoc_return_type_code;
Section* (*nrnpy_o2sec_p_)(Object* o);

void (*nrnpy_sectionlist_helper_)(List*, Object*) = 0;

void lvappendsec_and_ref(void* sl, Section* sec) {
    lappendsec((List*) sl, sec);
    section_ref(sec);
}

/*ARGSUSED*/
static void* constructor(Object* ho) {
    List* sl;
    sl = newlist();
    if (nrnpy_sectionlist_helper_ && ifarg(1)) {
        nrnpy_sectionlist_helper_(sl, *hoc_objgetarg(1));
    }
    return (void*) sl;
}

static void destructor(void* v) {
    Item* q;
    List* sl = (List*) v;
    ITERATE(q, sl) {
        section_unref(q->element.sec);
    }
    freelist(&sl);
}

Section* nrn_secarg(int i) {
#if USE_PYTHON
    if (ifarg(i) && nrnpy_o2sec_p_) {
        Object* o = *hoc_objgetarg(i);
        return (*nrnpy_o2sec_p_)(o);
    }
#endif
    return chk_access();
}

static double append(void* v) {
    Section* sec = nrn_secarg(1);
    if (ifarg(2)) {
        hoc_execerror("Too many parameters. SectionList.append takes 0 or 1 arguments", (char*) 0);
    }
    lvappendsec_and_ref(v, sec);
    return 1.;
}


static Item* children1(List* sl, Section* sec) {
    Item* i;
    Section* ch;
    i = sl->prev;
    for (ch = sec->child; ch; ch = ch->sibling) {
        i = lappendsec(sl, ch);
        section_ref(ch);
    }
    return i;
}

static double children(void* v) {
    Section* sec;
    List* sl;
    sec = nrn_secarg(1);
    sl = (List*) v;
    children1(sl, sec);
    return 1.;
}

static Item* subtree1(List* sl, Section* sec) {
    Item *i, *j, *last, *first;
    Section* s;
    /* it is confusing to span the tree from the root without using
      recursion.
    */
    s = sec;
    i = lappendsec(sl, s);
    section_ref(s);
    last = i->prev;
    while (i != last) {
        for (first = last->next, last = i, j = first; j->prev != last; j = j->next) {
            s = hocSEC(j);
            i = children1(sl, s);
        }
    }
    return i;
}

static double subtree(void* v) {
    Section* sec;
    List* sl;
    sec = nrn_secarg(1);
    sl = (List*) v;
    subtree1(sl, sec);
    return 1.;
}

static double wholetree(void* v) {
    List* sl;
    Section *s, *sec, *ch;
    Item *i, *j, *first, *last;
    sec = nrn_secarg(1);
    sl = (List*) v;
    /*find root*/
    for (s = sec; s->parentsec; s = s->parentsec) {
    }

    subtree1(sl, s);
    return 1.;
}

static double allroots(void* v) {
    List* sl;
    Item* qsec;
    sl = (List*) v;
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        if (!sec->parentsec) {
            lappendsec(sl, sec);
            section_ref(sec);
        }
    }

    return 1.;
}

static double seclist_remove(void* v) {
    Section *sec, *s;
    Item *q, *q1;
    List* sl;
    int i;

    sl = (List*) v;
    i = 0;
#if USE_PYTHON
    if (!ifarg(1) || (*hoc_objgetarg(1))->ctemplate->sym == nrnpy_pyobj_sym_) {
#else
    if (!ifarg(1)) {
#endif
        sec = nrn_secarg(1);
        ITERATE_REMOVE(q, q1, sl) /*{*/
        if (sec == q->element.sec) {
            hoc_l_delete(q);
            section_unref(sec);
            return 1.;
        }
    }
    hoc_warning(secname(sec), "not in this section list");
}
else {
    Object* o;
    o = *hoc_objgetarg(1);
    check_obj_type(o, "SectionList");
    ITERATE_REMOVE(q, q1, sl) /*{*/
    s = hocSEC(q);
    s->volatile_mark = 0;
}
sl = (List*) o->u.this_pointer;
ITERATE_REMOVE(q, q1, sl) /*{*/
s = hocSEC(q);
s->volatile_mark = 1;
}
sl = (List*) v;
i = 0;
for (q = sl->next; q != sl; q = q1) {
    q1 = q->next;
    s = hocSEC(q);
    if (s->volatile_mark) {
        hoc_l_delete(q);
        section_unref(s);
        ++i;
    }
}
}
return (double) i;
}

static double unique(void* v) {
    int i; /* number deleted */
    Section* s;
    Item *q, *q1;
    List* sl = (List*) v;
    hoc_return_type_code = 1; /* integer */
    ITERATE_REMOVE(q, q1, sl) /*{*/
    s = hocSEC(q);
    s->volatile_mark = 0;
}
i = 0;
for (q = sl->next; q != sl; q = q1) {
    q1 = q->next;
    s = hocSEC(q);
    if (s->volatile_mark++) {
        hoc_l_delete(q);
        section_unref(s);
        ++i;
    }
}
return (double) i;
}

static double contains(void* v) {
    Section* s;
    Item *q, *q1;
    List* sl = (List*) v;
    hoc_return_type_code = 2; /* boolean */
    s = nrn_secarg(1);
    ITERATE_REMOVE(q, q1, sl) /*{*/
    if (hocSEC(q) == s) {
        return 1.;
    }
}
return (0.);
}

static double printnames(void* v) {
    Item *q, *q1;
    List* sl = (List*) v;
    ITERATE_REMOVE(q, q1, sl) /*{*/
    if (q->element.sec->prop) {
        Printf("%s\n", secname(q->element.sec));
    }
}
return 1.;
}

static Member_func members[] = {{"append", append},
                                {"remove", seclist_remove},
                                {"wholetree", wholetree},
                                {"subtree", subtree},
                                {"children", children},
                                {"unique", unique},
                                {"printnames", printnames},
                                {"contains", contains},
                                {"allroots", allroots},
                                {0, 0}};


extern void class2oc(const char*,
                     void* (*cons)(Object*),
                     void (*destruct)(void*),
                     Member_func*,
                     int (*checkpoint)(void**),
                     Member_ret_obj_func*,
                     Member_ret_str_func*);


void SectionList_reg(void) {
    /*	printf("SectionList_reg\n");*/
    class2oc("SectionList", constructor, destructor, members, nullptr, nullptr, nullptr);
}

#define relative(pc) (pc + (pc)->i)
extern int hoc_returning;

static void check(Object* ob) {
    if (!ob) {
        hoc_execerror("nullptr object is not a SectionList", nullptr);
    }
    if (ob->ctemplate->constructor != constructor) {
        hoc_execerror(ob->ctemplate->sym->name, " is not a SectionList");
    }
}

void forall_sectionlist(void) {
    Inst* savepc = pc;
    Item *q, *q1;
    Section* sec;
    List* sl;
    Object* ob;
    Object** obp;
    int istk;

    /* if arg is a string use forall_section */
    if (hoc_stacktype() == STRING) {
        forall_section();
        return;
    }
    obp = hoc_objpop();
    ob = *obp;
    check(ob);
    sl = (List*) (ob->u.this_pointer);
    istk = nrn_isecstack();
    for (q = sl->next; q != sl; q = q1) {
        q1 = q->next;
        sec = q->element.sec;
        if (!sec->prop) {
            hoc_l_delete(q);
            section_unref(sec);
            continue;
        }
        nrn_pushsec(sec);
        hoc_execute(relative(savepc));
        nrn_popsec();
        if (hoc_returning) {
            nrn_secstack(istk);
        }
        if (hoc_returning == 1 || hoc_returning == 4) {
            break;
        } else if (hoc_returning == 2) {
            hoc_returning = 0;
            break;
        } else {
            hoc_returning = 0;
        }
    }
    hoc_tobj_unref(obp);
    if (!hoc_returning) {
        pc = relative(savepc + 1);
    }
}

void hoc_ifseclist(void) {
    Inst* savepc = pc;
    Item *q, *q1;
    Section* sec = chk_access();
    List* sl;
    Object* ob;
    Object** obp;

    /* if arg is a string use forall_section */
    if (hoc_stacktype() == STRING) {
        hoc_ifsec();
        return;
    }
    obp = hoc_objpop();
    ob = *obp;
    check(ob);
    sl = (List*) (ob->u.this_pointer);
    ITERATE_REMOVE(q, q1, sl) /*{*/
    if (sec == q->element.sec) {
        hoc_execute(relative(savepc));
        if (!hoc_returning) {
            pc = relative(savepc + 1);
        }
        hoc_tobj_unref(obp);
        return;
    }
}
hoc_tobj_unref(obp);
if (!hoc_returning) {
    pc = relative(savepc + 1);
}
}

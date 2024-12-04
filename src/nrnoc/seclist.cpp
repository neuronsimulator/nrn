#include <../../nrnconf.h>

#include <functional>

#define HOC_L_LIST 1
#include "section.h"
#include "neuron.h"
#include "nrnpy.h"
#include "parse.hpp"
#include "hocparse.h"
#include "code.h"
#include "classreg.h"

template <typename F>
bool seclist_iterate_remove_until(List* sl, F fun, const Section* sec) {
    Item* q2 = nullptr;
    for (Item* q1 = sl->next; q1 != sl; q1 = q2) {
        q2 = q1->next;
        if (q1->element.sec->prop == nullptr) {
            hoc_l_delete(q1);
            continue;
        }
        if (q1->element.sec == sec) {
            fun(q1);
            return true;
        }
    }
    return false;
}

template <typename F>
void seclist_iterate_remove(List* sl, F fun) {
    Item* q2 = nullptr;
    for (Item* q1 = sl->next; q1 != sl; q1 = q2) {
        q2 = q1->next;
        if (q1->element.sec->prop == nullptr) {
            hoc_l_delete(q1);
            continue;
        }
        fun(q1->element.sec);
    }
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

static void children1(List* sl, Section* sec) {
    for (Section* ch = sec->child; ch; ch = ch->sibling) {
        lappendsec(sl, ch);
        section_ref(ch);
    }
}

static double children(void* v) {
    Section* sec;
    List* sl;
    sec = nrn_secarg(1);
    sl = (List*) v;
    children1(sl, sec);
    return 1.;
}

// Create a list of section from the subtree with root `sec` by
// doing breadth-first traversal.
static void subtree1(List* sl, Section* sec) {
    const Item* end = sl;
    // The pointer `sl` points to a past-the-end
    // marker. Therefore, appending items means
    // they appear immediately before `sl`.
    Item* current = lappendsec(sl, sec);
    section_ref(sec);
    while (current != end) {
        children1(sl, hocSEC(current));
        current = current->next;
    }
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
    Section* sec = nrn_secarg(1);
    List* sl = (List*) v;
    /*find root*/
    Section* s = nullptr;
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
    List* sl = (List*) v;
    int i = 0;
#if USE_PYTHON
    if (!ifarg(1) || (*hoc_objgetarg(1))->ctemplate->sym == nrnpy_pyobj_sym_) {
#else
    if (!ifarg(1)) {
#endif
        Section* sec = nrn_secarg(1);
        if (seclist_iterate_remove_until(
                sl,
                [](Item* q) {
                    Section* s = q->element.sec;
                    hoc_l_delete(q);
                    section_unref(s);
                },
                sec)) {
            return 1.;
        }
        hoc_warning(secname(sec), "not in this section list");
    } else {
        Object* o;
        o = *hoc_objgetarg(1);
        check_obj_type(o, "SectionList");
        seclist_iterate_remove(sl, [](Section* s) { s->volatile_mark = 0; });
        sl = (List*) o->u.this_pointer;
        seclist_iterate_remove(sl, [](Section* s) { s->volatile_mark = 1; });
        sl = (List*) v;
        Item* q1;
        for (Item* q = sl->next; q != sl; q = q1) {
            q1 = q->next;
            Section* s = hocSEC(q);
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
    Item* q1;
    List* sl = (List*) v;
    hoc_return_type_code = 1; /* integer */
    seclist_iterate_remove(sl, [](Section* s) { s->volatile_mark = 0; });
    int i = 0; /* number deleted */
    for (Item* q = sl->next; q != sl; q = q1) {
        q1 = q->next;
        Section* s = hocSEC(q);
        if (s->volatile_mark++) {
            hoc_l_delete(q);
            section_unref(s);
            ++i;
        }
    }
    return (double) i;
}

static double contains(void* v) {
    List* sl = (List*) v;
    hoc_return_type_code = 2; /* boolean */
    Section* s = nrn_secarg(1);
    return seclist_iterate_remove_until(
               sl, [](Item*) {}, s)
               ? 1.
               : 0.;
}

static double printnames(void* v) {
    List* sl = (List*) v;
    seclist_iterate_remove(sl, [](Section* s) {
        if (s->prop) {
            Printf("%s\n", secname(s));
        }
    });
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
                                {nullptr, nullptr}};

void SectionList_reg(void) {
    /*	printf("SectionList_reg\n");*/
    class2oc("SectionList", constructor, destructor, members, nullptr, nullptr);
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
    Section* sec = chk_access();

    /* if arg is a string use forall_section */
    if (hoc_stacktype() == STRING) {
        hoc_ifsec();
        return;
    }
    Object** obp = hoc_objpop();
    Object* ob = *obp;
    check(ob);
    List* sl = (List*) (ob->u.this_pointer);
    if (seclist_iterate_remove_until(
            sl,
            [&](Item*) {
                hoc_execute(relative(savepc));
                if (!hoc_returning) {
                    pc = relative(savepc + 1);
                }
                hoc_tobj_unref(obp);
            },
            sec)) {
        return;
    }
    hoc_tobj_unref(obp);
    if (!hoc_returning) {
        pc = relative(savepc + 1);
    }
}

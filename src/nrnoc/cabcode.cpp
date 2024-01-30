#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/cabcode.cpp,v 1.37 1999/07/08 14:24:59 hines Exp */

#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define HOC_L_LIST 1
#include "section.h"
#include "nrn_ansi.h"
#include "nrniv_mf.h"
#include "membfunc.h"
#include "parse.hpp"
#include "hocparse.h"
#include "membdef.h"

static char* escape_bracket(const char* s) {
    static char* b;
    const char* p1;
    char* p2;
    if (!b) {
        b = new char[256];
    }
    for (p1 = s, p2 = b; *p1; ++p1, ++p2) {
        switch (*p1) {
        case '<':
            *p2 = '[';
            break;
        case '>':
            *p2 = ']';
            break;
        case '[':
        case ']':
            *p2 = '\\';
            *(++p2) = *p1;
            break;
        default:
            *p2 = *p1;
            break;
        }
    }
    *p2 = '\0';
    return b;
}


extern int hoc_execerror_messages;
#define symlist hoc_symlist

int tree_changed = 1; /* installing section, changeing nseg
              and connecting sections set this flag.
              The flag is set to 0 when the topology
              is set up */
int diam_changed = 1; /* changing diameter, length set this flag
              The flag is set to 0 when node.a and node.b
              is set up */
extern int nrn_shape_changed_;

char* (*nrnpy_pysec_name_p_)(Section*);
Object* (*nrnpy_pysec_cell_p_)(Section*);
int (*nrnpy_pysec_cell_equals_p_)(Section*, Object*);

/* switching from Ra being a global variable to it being a section variable
opens up the possibility of a great deal of confusion and inadvertant wrong
results. To help avoid this we print a warning message whenever the value
in one section is set but no others. But only the first time through treeset.
*/
#if RA_WARNING
int nrn_ra_set = 0;
#endif

#define NSECSTACK 200
static Section* secstack[NSECSTACK + 1];
static int isecstack = 0; /* stack index */
/* don't do section stack auto correction
 in the interval between push_section() ... pop_section in hoc (also for
 point process get_loc. These should be deprecated in favor of doing
 everything with SectionRef
*/
static int skip_secstack_check = 0;

static int range_vec_indx(Symbol* s);

int nrn_isecstack(void) {
    return isecstack;
}

void nrn_secstack(int i) {
    if (skip_secstack_check) {
        return;
    }
#if 1
    if (isecstack > i) {
        Printf("The sectionstack index should be %d but it is %d\n", i, isecstack);
        hoc_warning(
            "prior to version 5.3 the section stack would not have been properly popped\n\
and the currently accessed section would have been ",
            secname(secstack[isecstack]));
    }
#endif
    while (isecstack > i) {
        nrn_popsec();
    }
}

void nrn_initcode(void) {
    while (isecstack > 0) {
        nrn_popsec();
    }
    isecstack = 0;
    section_object_seen = 0;
    state_discon_allowed_ = 1;
    skip_secstack_check = 0;
}

void oc_save_cabcode(int* a1, int* a2) {
    *a1 = isecstack;
    *a2 = section_object_seen;
}

void oc_restore_cabcode(int* a1, int* a2) {
    while (isecstack > *a1) {
        nrn_popsec();
    }
    isecstack = *a1;
    section_object_seen = *a2;
}

void nrn_pushsec(Section* sec) {
    isecstack++;
    if (isecstack >= NSECSTACK) {
        int i = NSECSTACK;
        hoc_warning("section stack overflow", (char*) 0);
        while (--i >= 0) {
            fprintf(stderr, "%*s%s\n", i, "", secname(secstack[i]));
            --i;
        }
        hoc_execerror("section stack overflow", (char*) 0);
    }
    secstack[isecstack] = sec;
    if (sec) {
#if 0
		if (sec->prop && sec->prop->dparam[0].sym) {
			printf("pushsec %s\n", sec->prop->dparam[0].sym->name);
		}else{
			printf("pushsec unnamed or deleted section\n");
		}
#endif
        ++sec->refcount;
    }
}

void nrn_popsec(void) {
    if (isecstack > 0) {
        Section* sec = secstack[isecstack--];
        if (!sec) {
            return;
        }
        section_unref(sec);
    }
}

void sec_access_pop(void) {
    nrn_popsec();
}

#if 0
static void free_point_process(void) {
	free_all_point();
	free_clamp();
	free_stim();
	free_syn();
}
#endif

#if 0
void clear_sectionlist(void)	/* merely change all SECTION to UNDEF */
{
printf("clear_sectionlist not fixed yet, doing nothing\n");
return;
	Symbol *s;

	free_point_process();
	if (symlist) for (s=symlist->first; s; s = s->next) {
		if (s->type == SECTION) {
			s->type = UNDEF;
no longer done this way see OPARINFO
			if (s->arayinfo) {
				free((char *)s->arayinfo);
				s->arayinfo = (Arrayinfo *)0;
			}
		}
	nrn_initcode();
	secstack[0] = (Section *)0;
	}
	if (section) {
		sec_free(section, nsection);
		section = (Section *)0;
		nsection = 0;
	}
}
#endif

void add_section(void) /* symbol at pc+1, number of indices at pc+2 */
{
    Symbol* sym;
    int nsub, size;
    Item** pitm;

    sym = (pc++)->sym;
    /*printf("declaring %s as section\n", sym->name);*/
    if (sym->type == SECTION) {
        int total, i;
        total = hoc_total_array(sym);
        for (i = 0; i < total; ++i) {
            sec_free(*(OPSECITM(sym) + i));
        }
        free((char*) OPSECITM(sym));
        hoc_freearay(sym);
    } else {
        assert(sym->type == UNDEF);
        if (hoc_objectdata != hoc_top_level_data && hoc_thisobject) {
            hoc_execerr_ext(
                "First time declaration of Section %s in %s "
                "must happen at command level (not in method)",
                sym->name,
                hoc_object_name(hoc_thisobject));
        }
        sym->type = SECTION;
        hoc_install_object_data_index(sym);
    }
    nsub = (pc++)->i;
    if (nsub) {
        size = hoc_arayinfo_install(sym, nsub);
    } else {
        size = 1;
    }
    hoc_objectdata[sym->u.oboff].psecitm = pitm = (Item**) emalloc(size * sizeof(Item*));
    if (hoc_objectdata == hoc_top_level_data) {
        new_sections((Object*) 0, sym, pitm, size);
    } else {
        new_sections(hoc_thisobject, sym, pitm, size);
    }
#if 0
printf("%s", s->name);
for (i=0; i<ndim; i++) {printf("[%d]",s->arayinfo->sub[i]);}
printf(" is a section name\n");
#endif
}

Object* nrn_sec2cell(Section* sec) {
    if (sec->prop) {
        if (auto* o = sec->prop->dparam[6].get<Object*>(); o) {
            return o;
        } else if (nrnpy_pysec_cell_p_) {
            Object* o = (*nrnpy_pysec_cell_p_)(sec);
            if (o) {
                --o->refcount;
            }
            return o;
        }
    }
    return nullptr;
}

int nrn_sec2cell_equals(Section* sec, Object* obj) {
    if (sec && sec->prop) {
        if (auto o = sec->prop->dparam[6].get<Object*>(); o) {
            return o == obj;
        } else if (nrnpy_pysec_cell_equals_p_) {
            return (*nrnpy_pysec_cell_equals_p_)(sec, obj);
        }
    }
    return 0;
}

static Section* new_section(Object* ob, Symbol* sym, int i) {
    Section* sec;
    Prop* prop;
    static Symbol* nseg;
    double d;

    if (!nseg) {
        nseg = hoc_lookup("nseg");
    }
    sec = sec_alloc();
    section_ref(sec);
    prop = prop_alloc(&(sec->prop), CABLESECTION, (Node*) 0);
    prop->dparam[0] = {neuron::container::do_not_search, sym};
    prop->dparam[5] = i;
    prop->dparam[6] = {neuron::container::do_not_search, ob};
#if USE_PYTHON
    prop->dparam[PROP_PY_INDEX] = nullptr;
#endif
    nrn_pushsec(sec);
    d = (double) DEF_nseg;
    cable_prop_assign(nseg, &d, 0);
    tree_changed = 1;
    /*printf("new_section %s\n", secname(sec));*/
    return sec;
}

void new_sections(Object* ob, Symbol* sym, Item** pitm, int size) {
    int i;
    for (i = 0; i < size; ++i) {
        Section* sec = new_section(ob, sym, i);
        if (ob) {
            if (ob->secelm_) {
                pitm[i] = insertsec(ob->secelm_->next, sec);
            } else {
                pitm[i] = lappendsec(section_list, sec);
            }
            ob->secelm_ = pitm[i];
        } else {
            pitm[i] = lappendsec(section_list, sec);
        }
        sec->prop->dparam[8] = {neuron::container::do_not_search, pitm[i]};
    }
}

#if USE_PYTHON
struct NPySecObj;
Section* nrnpy_newsection(NPySecObj* v) {
    Item* itm;
    Section* sec;
    sec = new_section((Object*) 0, (Symbol*) 0, 0);
#if USE_PYTHON
    sec->prop->dparam[PROP_PY_INDEX] = static_cast<void*>(v);
#endif
    itm = lappendsec(section_list, sec);
    sec->prop->dparam[8] = itm;
    return sec;
}
#endif

void delete_section(void) {
    Section* sec;
    Item** pitm;
    if (ifarg(1)) {
        hoc_execerror(
            "delete_section takes no positional arguments and deletes the HOC currently accessed "
            "section. If using Python, did you mean a named arg of the form, sec=section?",
            NULL);
    }
    sec = chk_access();
    if (!sec->prop) { /* already deleted */
        hoc_retpushx(0.0);
        return;
    }
#if USE_PYTHON
    if (sec->prop->dparam[PROP_PY_INDEX].get<void*>()) { /* Python Section */
        /* the Python Section will be a zombie section with a pointer
           to an invalid Section*.
        */
        sec->prop->dparam[PROP_PY_INDEX] = nullptr;
        section_ref(sec);
        sec_free(sec->prop->dparam[8].get<hoc_Item*>());
        hoc_retpushx(0.0);
        return;
    }
#endif
    auto* sym = sec->prop->dparam[0].get<Symbol*>();
    if (!sym) {
        hoc_execerror("Cannot delete an unnamed hoc section", (char*) 0);
    }
    auto* ob = sec->prop->dparam[6].get<Object*>();
    auto i = sec->prop->dparam[5].get<int>();
    if (ob) {
        pitm = ob->u.dataspace[sym->u.oboff].psecitm + i;
    } else {
        pitm = hoc_top_level_data[sym->u.oboff].psecitm + i;
    }
    sec_free(*pitm);
    *pitm = 0;
    hoc_retpushx(1.);
}

/*
At this point, all the sections are cables and each section has the following
properties (except for section 0). Only one property with 9 Datums

section[i].prop->dparam[0].sym 	pointer to section symbol
            [1].val	position (0--1) of connection to parent
            [2].val	length of section in microns
            [3].val	first node at position 0 or 1
            [4].val rall branch
            [5].i	aray index
            [6].obj pointer to the object pointer
            [7].val Ra
            [8].itm hoc_Item* with Section* as element
            [9]._pvoid NrnThread*
            [PROP_PY_INDEX].pvoid pointer to the Python Section object
The first element allows us to find the symbol when we know the section number.
If an array section the index can be computed with
i - (section[i].sym)->u.u_auto
*/

double section_length(Section* sec) {
    if (sec->recalc_area_ && sec->npt3d) {
        sec->prop->dparam[2] = sec->pt3d[sec->npt3d - 1].arc;
    }
    double x = sec->prop->dparam[2].get<double>();
    if (x <= 1e-9) {
        x = 1e-9;
        sec->prop->dparam[2] = x;
    }
    return x;
}

int arc0at0(Section* sec) {
    return (sec->prop->dparam[3].get<double>() ? 0 : 1);
}

double nrn_ra(Section* sec) {
    return sec->prop->dparam[7].get<double>();
}

void cab_alloc(Prop* p) {
    Datum* pd;
#if USE_PYTHON
#define CAB_SIZE 11
#else
#define CAB_SIZE 10
#endif
    pd = nrn_prop_datum_alloc(CABLESECTION, CAB_SIZE, p);
    pd[1] = 0.0;
    pd[2] = DEF_L;
    pd[3] = 0.0;
    pd[4] = DEF_rallbranch;
    pd[7] = DEF_Ra;
    p->dparam = pd;
    p->dparam_size = CAB_SIZE; /* this one is special since it refers to dparam */
}

void morph_alloc(Prop* p) {
    assert(p->param_size() == 1);
    p->param(0) = DEF_diam; /* microns */
    diam_changed = 1;
}

double nrn_diameter(Node* nd) {
    Prop* p = nrn_mechanism(MORPHOLOGY, nd);
    return p->param(0);
}

void nrn_chk_section(Symbol* s) {
    if (s->type != SECTION) {
        execerror("Not a SECTION name:", s->name);
    }
}

Section* chk_access() {
    Section* sec = secstack[isecstack];
    if (!sec || !sec->prop) {
        /* use any existing section as a default section */
        hoc_Item* qsec;
        // ForAllSections(lsec)
        ITERATE(qsec, section_list) {
            Section* lsec = hocSEC(qsec);
            if (lsec->prop) {
                sec = lsec;
                ++sec->refcount;
                secstack[isecstack] = sec;
                /*printf("automatic default section %s\n", secname(sec));*/
                break;
            }
        }
    }
    if (!sec) {
        execerror("Section access unspecified", (char*) 0);
    }
    if (sec->prop) {
        return sec;
    } else {
        execerror("Accessing a deleted section", (char*) 0);
    }
    return NULL; /* never reached */
}

Section* nrn_noerr_access(void) /* return 0 if no accessed section */
{
    Section* sec = secstack[isecstack];
    if (!sec || !sec->prop) {
        /* use any existing section as a default section */
        hoc_Item* qsec;
        // ForAllSections(lsec)
        ITERATE(qsec, section_list) {
            Section* lsec = hocSEC(qsec);
            if (lsec->prop) {
                sec = lsec;
                ++sec->refcount;
                secstack[isecstack] = sec;
                /*printf("automatic default section %s\n", secname(sec));*/
                break;
            }
        }
    }
    if (!sec) {
        return (Section*) 0;
    }
    if (sec->prop) {
        return sec;
    } else {
        return (Section*) 0;
    }
}

/*sibling and child pointers do not ref sections to avoid mutual references */
/* the sibling list is ordered according to increasing distance from parent */

void nrn_remove_sibling_list(Section* sec) {
    Section* s;
    if (sec->parentsec) {
        if (sec->parentsec->child == sec) {
            sec->parentsec->child = sec->sibling;
            return;
        }
        for (s = sec->parentsec->child; s; s = s->sibling) {
            if (s->sibling == sec) {
                s->sibling = sec->sibling;
                return;
            }
        }
    }
}

static double ncp_abs(Section* sec) {
    double x = nrn_connection_position(sec);
    if (sec->parentsec) {
        if (!arc0at0(sec->parentsec)) {
            return 1. - x;
        }
    }
    return x;
}

void nrn_add_sibling_list(Section* sec) {
    Section* s;
    double x;
    if (sec->parentsec) {
        x = ncp_abs(sec);
        s = sec->parentsec->child;
        if (!s || x <= ncp_abs(s)) {
            sec->sibling = s;
            sec->parentsec->child = sec;
            return;
        }
        for (; s->sibling; s = s->sibling) {
            if (x <= ncp_abs(s->sibling)) {
                sec->sibling = s->sibling;
                s->sibling = sec;
                return;
            }
        }
        s->sibling = sec;
        sec->sibling = 0;
    }
}

static void reverse_sibling_list(Section* sec) {
    int scnt;
    Section* ch;
    Section** pch;
    for (scnt = 0, ch = sec->child; ch; ++scnt, ch = ch->sibling) {
        hoc_pushobj((Object**) ch);
    }
    pch = &sec->child;
    while (scnt--) {
        ch = (Section*) hoc_objpop();
        *pch = ch;
        pch = &ch->sibling;
        ch->parentnode = 0;
    }
    *pch = 0;
}

void disconnect(void) {
    if (ifarg(1)) {
        hoc_execerror(
            "disconnect takes no positional arguments and disconnects the HOC currently accessed "
            "section. If using Python, did you mean a named arg of the form, sec=section? Or you "
            "can use section.disconnect().",
            NULL);
    }
    nrn_disconnect(chk_access());
    hoc_retpushx(0.);
}

static void reverse_nodes(Section* sec) {
    int i, j;
    Node* nd;
    /*	printf("reverse %d nodes for %s\n", sec->nnode-1, secname(sec));*/
    for (i = 0, j = sec->nnode - 2; i < j; ++i, --j) {
        nd = sec->pnode[i];
        sec->pnode[i] = sec->pnode[j];
        sec->pnode[i]->sec_node_index_ = i;
        sec->pnode[j] = nd;
        nd->sec_node_index_ = j;
    }
}

void nrn_disconnect(Section* sec) {
    Section* ch;
    Section* oldpsec = sec->parentsec;
    Node* oldpnode = sec->parentnode;
    if (!oldpsec) {
        return;
    }
    nrn_remove_sibling_list(sec);
    sec->parentsec = (Section*) 0;
    sec->parentnode = (Node*) 0;
    nrn_parent_info(sec);
    nrn_relocate_old_points(sec, oldpnode, sec, sec->parentnode);
    for (ch = sec->child; ch; ch = ch->sibling)
        if (nrn_at_beginning(ch)) {
            ch->parentnode = sec->parentnode;
            nrn_relocate_old_points(ch, oldpnode, ch, ch->parentnode);
        }
    section_unref(oldpsec);
    tree_changed = 1;
    neuron::model().node_data().mark_as_unsorted();
}

static void connectsec_impl(Section* parent, Section* sec) {
    Section* ch;
    Section* oldpsec = sec->parentsec;
    Node* oldpnode = sec->parentnode;
    double d1, d2;
    Datum* pd;
    d2 = xpop();
    d1 = xpop();
    if (d1 != 0. && d1 != 1.) {
        hoc_execerror(secname(sec), " must connect at position 0 or 1");
    }
    if (d2 < 0. || d2 > 1.) {
        hoc_execerror(secname(sec), " must connect from 0<=x<=1 of parent");
    }
    if (sec->parentsec) {
        fprintf(stderr, "Notice: %s(%g)", secname(sec), nrn_section_orientation(sec));
        fprintf(stderr,
                " had previously been connected to parent %s(%g)\n",
                secname(sec->parentsec),
                nrn_connection_position(sec));
        nrn_remove_sibling_list(sec);
    }
    if (nrn_section_orientation(sec) != d1) {
        reverse_sibling_list(sec);
        reverse_nodes(sec);
    }
    pd = sec->prop->dparam;
    pd[1] = d2;
    pd[3] = d1;
    section_ref(parent);
    sec->parentsec = parent;
    nrn_add_sibling_list(sec);
    sec->parentnode = (Node*) 0;
    nrn_parent_info(sec);
    nrn_relocate_old_points(sec, oldpnode, sec, sec->parentnode);
    for (ch = sec->child; ch; ch = ch->sibling)
        if (nrn_at_beginning(ch)) {
            ch->parentnode = sec->parentnode;
            nrn_relocate_old_points(ch, oldpnode, ch, ch->parentnode);
        }
    if (oldpsec) {
        section_unref(oldpsec);
    } else if (oldpnode) {
        delete oldpnode;
    }
    tree_changed = 1;
    diam_changed = 1;
}

void simpleconnectsection(void) /* 2 expr on stack and two sections on section stack */
                                /* for a prettier syntax: connect sec1(x), sec2(x) */
{
    Section *parent, *child;
    parent = nrn_sec_pop();
    child = nrn_sec_pop();
    connectsec_impl(parent, child);
}

void connectsection(void) /* 2 expr on stack and section symbol on section stack */
{
    Section *parent, *child;
    child = nrn_sec_pop();
    parent = chk_access();
    connectsec_impl(parent, child);
}

static Section* Sec_access(void) /* section symbol at pc */
{
    Objectdata* odsav;
    Object* obsav = 0;
    Symlist* slsav;
    Item* itm;
    Symbol* s = (pc++)->sym;

    if (!s) {
        return chk_access();
    }

    if (s->cpublic == 2) {
        s = s->u.sym;
        odsav = hoc_objectdata_save();
        obsav = hoc_thisobject;
        slsav = hoc_symlist;
        hoc_objectdata = hoc_top_level_data;
        hoc_thisobject = 0;
        hoc_symlist = hoc_top_level_symlist;
    }
    nrn_chk_section(s);
    itm = OPSECITM(s)[range_vec_indx(s)];
    if (obsav) {
        hoc_objectdata = hoc_objectdata_restore(odsav);
        hoc_thisobject = obsav;
        hoc_symlist = slsav;
    }
    if (!itm) {
        hoc_execerror(s->name, ": section was deleted");
    }
    return itm->element.sec;
#if 0
printf("accessing %s with index %d\n", s->name, access_index);
#endif
}

void sec_access(void) { /* access section */
    Section** psec;
    Section* sec = chk_access();
    ++sec->refcount;
    nrn_popsec();
    psec = secstack + isecstack;
    if (*psec) {
        section_unref(*psec);
    }
    *psec = sec;
}

void sec_access_object(void) { /* access section */
    Section** psec;
    Section* sec;
    if (!section_object_seen) {
        hoc_execerror("Access: Not a section", (char*) 0);
    }
    sec = chk_access();
    ++sec->refcount;
    nrn_popsec();
    psec = secstack + isecstack;
    if (*psec) {
        section_unref(*psec);
    }
    *psec = sec;
    section_object_seen = 0;
}

void sec_access_push(void) {
    nrn_pushsec(Sec_access());
}

Section* nrn_sec_pop(void) {
    Section* sec = chk_access();
    nrn_popsec();
    return sec;
}

void hoc_sec_internal_push(void) {
    Section* sec = (Section*) ((pc++)->ptr);
    nrn_pushsec(sec);
}

void* hoc_sec_internal_name2ptr(const char* s, int eflag) {
    /*
      syntax is __nrnsec_0xff... and need to verify that ff... is a pointer
      to a Section. To avoid invalid memory read errors, we changed
      the section allocation/free in solve.cpp to use a SectionPool which
      allows checking to see if a void* is a possible Section* from
      the pool.
    */
    int n;
    Section* sec;
    void* vp = NULL;
    int err = 0;
    n = strlen(s);
    if (n < 12 || strncmp(s, "__nrnsec_0x", 11) != 0) {
        err = 1;
    } else {
        if (sscanf(s + 9, "%p", &vp) != 1) {
            err = 1;
        }
    }
    if (err) {
        if (eflag) {
            hoc_execerror("Invalid internal section name:", s);
        } else {
            hoc_warning("Invalid internal section name:", s);
        }
        return NULL;
    }
    sec = (Section*) vp;
    if (nrn_is_valid_section_ptr(vp) == 0 || !sec->prop || !sec->prop->dparam ||
        !sec->prop->dparam[8].get<hoc_Item*>() ||
        sec->prop->dparam[8].get<hoc_Item*>()->itemtype != SECTION) {
        if (eflag) {
            hoc_execerror("Section associated with internal name does not exist:", s);
        } else {
            hoc_warning("Section associated with internal name does not exist:", s);
        }
        vp = NULL;
    }
    return vp;
}

void* hoc_pysec_name2ptr(const char* s, int eflag) {
    /*
      syntax is _pysec.<name>  where <name> is the name of a python
      nrn.Section from (*nrnpy_pysec_name_p_)(sec)

    */
    Section* sec = nrnpy_pysecname2sec(s);
    return (void*) sec;
}

/* in an object syntax a section may either be last or next to last
in either case it is pushed when it is seen in hoc_object_component
and section_object_seen is set.
If it was last then this is called with it set and we do nothing.
ie the section is on the stack till the end of the next statement.
If it was second to last then it hoc_object_component will set it
back to 0 and pop the section. Therefore we need to push a section
below to keep the stack ok when it is popped at the end of the next
statement.
*/

void ob_sec_access_push(Item* qsec) {
    if (!qsec) {
        hoc_execerror("section in the object was deleted", (char*) 0);
    }
    nrn_pushsec(qsec->element.sec);
}

void ob_sec_access(void) {
    if (!section_object_seen) {
        hoc_nopop();
        nrn_pushsec(secstack[isecstack]);
    }
    section_object_seen = 0;
}

/* For now there is always one more node than segments and the last
node has no properties. This fact is spread everywhere in which
nnode is dealt with */

void mech_access(void) { /* section symbol at pc */
    Section* sec = chk_access();
    Symbol* s = (pc++)->sym;
    mech_insert1(sec, s->subtype);
}

void mech_insert1(Section* sec, int type) {
    int n, i;
    Node *nd, **pnd;
    Prop* m;
    /* make sure that all nodes but last of the current section have this
       membrane type */
    /* subsequent range variables refer to this mechanism */
    /* For now, we assume:
       1) Parameters for different mechanisms do not have to
          have distinct names. Thus access mech marks those
          symbols which are allowed.
          Another access section gets back to the geometry properties.
       2) Membrane is installed in entire section. (except last node)
    */
    /* Is the property already allocated*/
    n = sec->nnode - 1;
    pnd = sec->pnode;
    nd = pnd[0];
    m = nrn_mechanism(type, nd);
    if (!m) { /* all nodes get the property */
        for (i = n - 1; i >= 0; i--) {
            IGNORE(prop_alloc(&(pnd[i]->prop), type, pnd[i]));
        }
#if EXTRACELLULAR
        if (type == EXTRACELL) {
            prop_alloc(&(pnd[n]->prop), type, pnd[n]); /*even last node*/
            /* if rootnode owned by section, it gets property as well*/
            if (!sec->parentsec) {
                nd = sec->parentnode;
                if (nd) {
                    prop_alloc(&nd->prop, type, nd);
                }
            }
            extcell_2d_alloc(sec);
            diam_changed = 1;
        }
#endif
    }
}

void mech_uninsert(void) {
    Section* sec = chk_access();
    Symbol* s = (pc++)->sym;
    mech_uninsert1(sec, s);
}

void mech_uninsert1(Section* sec, Symbol* s) {
    /* remove the mechanism from the currently accessed section */
    int n, i;
    Prop *m, *mnext;
    int type = s->subtype;

    if (type == MORPHOLOGY
#if EXTRACELLULAR
        || type == EXTRACELL
#endif
    ) {
        hoc_warning("Can't uninsert mechanism", s->name);
        return;
    }
    if (nrn_is_ion(type)) {
        hoc_warning("Not allowed to uninsert ions at this time", s->name);
        return;
    }
    n = sec->nnode;
    for (i = 0; i < n; ++i) {
        mnext = sec->pnode[i]->prop;
        if (mnext && mnext->_type == type) {
            sec->pnode[i]->prop = mnext->next;
            single_prop_free(mnext);
            continue;
        }
        for (m = mnext; m; m = mnext) {
            mnext = m->next;
            if (mnext && mnext->_type == type) {
                m->next = mnext->next;
                single_prop_free(mnext);
                break;
            }
        }
    }
}

void nrn_rangeconst(Section* sec, Symbol* s, neuron::container::data_handle<double> pd, int op) {
    short n, i;
    Node* nd;
    int indx;
    neuron::container::data_handle<double> dpr{};
    double d = *pd;
    n = sec->nnode - 1;
    if (s->u.rng.type == VINDEX) {
        nd = node_ptr(sec, 0., (double*) 0);
        if (op) {
            *pd = hoc_opasgn(op, NODEV(nd), d);
        }
        nd->v() = *pd;
        nd = node_ptr(sec, 1., (double*) 0);
        if (op) {
            *pd = hoc_opasgn(op, NODEV(nd), d);
        }
        nd->v() = *pd;
        for (i = 0; i < n; i++) {
            if (op) {
                *pd = hoc_opasgn(op, NODEV(sec->pnode[i]), d);
            }
            sec->pnode[i]->v() = *pd;
        }
    } else {
        if (s->u.rng.type == IMEMFAST) {
            hoc_execerror("i_membrane_ cannot be assigned a value", 0);
        }
        indx = range_vec_indx(s);
        if (s->u.rng.type == MORPHOLOGY) {
            if (!can_change_morph(sec)) {
                return;
            }
            diam_changed = 1;
            if (sec->recalc_area_ && op != 0) {
                nrn_area_ri(sec);
            }
        }

        for (i = 0; i < n; i++) {
            dpr = dprop(s, indx, sec, i);
            if (op) {
                *pd = hoc_opasgn(op, *dpr, d);
            }
            *dpr = *pd;
        }
        if (s->u.rng.type == MORPHOLOGY) {
            sec->recalc_area_ = 1;
            nrn_diam_change(sec);
        }
#if EXTRACELLULAR
        if (s->u.rng.type == EXTRACELL) {
            if (s->u.rng.index == 0) {
                diam_changed = 1;
            }
            dpr = neuron::container::data_handle<double>{
                nrn_vext_pd(s, indx, node_ptr(sec, 0., nullptr))};
            if (dpr) {
                if (op) {
                    *dpr = hoc_opasgn(op, *dpr, d);
                } else {
                    *dpr = d;
                }
            }
            dpr = neuron::container::data_handle<double>{
                nrn_vext_pd(s, indx, node_ptr(sec, 1., nullptr))};
            if (dpr) {
                if (op) {
                    *dpr = hoc_opasgn(op, *dpr, d);
                } else {
                    *dpr = d;
                }
            }
        }
#endif
    }
}

// rangevariable symbol at pc, value on stack
void range_const() {
    Symbol* s = (hoc_pc++)->sym;
    int const op{(hoc_pc++)->i};
    double d{hoc_xpop()};
    auto* const sec = nrn_sec_pop();
    nrn_rangeconst(sec,
                   s,
                   neuron::container::data_handle<double>{neuron::container::do_not_search, &d},
                   op);
    hoc_pushx(d);
}

static int range_vec_indx(Symbol* s) {
    int indx;

    if (ISARRAY(s)) {
        indx = hoc_araypt(s, SYMBOL);
    } else {
        indx = 0;
    }
    return indx;
}

/* returns property for mechanism at the node */
Prop* nrn_mechanism(int type, Node* nd) {
    Prop* m;
    for (m = nd->prop; m; m = m->next) {
        if (m->_type == type) {
            break;
        }
    }
    return m;
}

/*returns prop given mech type, section, and inode */
/* error if mech not at this position */
Prop* nrn_mechanism_check(int type, Section* sec, int inode) {
    Prop* m;
    m = nrn_mechanism(type, sec->pnode[inode]);
    if (!m) {
        if (hoc_execerror_messages) {
            Fprintf(stderr,
                    "%s mechanism not inserted in section %s\n",
                    memb_func[type].sym->name,
                    secname(sec));
        }
        hoc_execerror("", (char*) 0);
    }
    return m;
}

Prop* hoc_getdata_range(int type) {
    int inode;
    Section* sec;
    double x;

    nrn_seg_or_x_arg(1, &sec, &x);
    inode = node_index(sec, x);
    return nrn_mechanism_check(type, sec, inode);
}

static Datum* pdprop(Symbol* s, int indx, Section* sec, short inode) {
    /* basically copied from dprop for use to connect a nmodl POINTER */
    Prop* m;

    m = nrn_mechanism_check(s->u.rng.type, sec, inode);
    return m->dparam + s->u.rng.index + indx;
}

// pointer symbol at pc, target variable on stack, maybe range variable location on stack
void connectpointer() {
    auto* const s = (hoc_pc++)->sym;
    auto const pd = hoc_pop_handle<double>();
    if (s->subtype != NRNPOINTER) {
        hoc_execerror(s->name, "not a model variable POINTER");
    }
    auto const d = hoc_xpop();
    auto* const sec = nrn_sec_pop();
    auto const i = node_index(sec, d);
    auto* const dat = pdprop(s, range_vec_indx(s), sec, i);
    *dat = pd;
}

void range_interpolate_single(void) /*symbol at pc, 2 values on stack*/
{
    double x, y;
    Symbol* s;
    Section* sec;
    int op;

    s = (pc++)->sym;
    op = (pc++)->i;
    y = xpop();
    x = xpop();
    sec = nrn_sec_pop();

    if (s->u.rng.type == MORPHOLOGY) {
        if (!can_change_morph(sec)) {
            return;
        }
        diam_changed = 1;
        if (sec->recalc_area_ && op != 0) {
            nrn_area_ri(sec);
        }
    }

    auto pd = nrn_rangepointer(sec, s, x);
    if (op) {
        y = hoc_opasgn(op, *pd, y);
    }
    *pd = y;

    if (s->u.rng.type == MORPHOLOGY) {
        sec->recalc_area_ = 1;
        nrn_diam_change(sec);
    }

#if EXTRACELLULAR
    if (s->u.rng.type == EXTRACELL && s->u.rng.index == 0) {
        diam_changed = 1;
    }
#endif
}

void range_interpolate(void) /*symbol at pc, 4 values on stack*/
{
    short i, i1, i2, di;
    Section* sec;
    double y1, y2, x1, x2, x, dx, thet, y;
    neuron::container::data_handle<double> dpr{};
    Symbol* s = (pc++)->sym;
    int indx, op;
    Node* nd;

    op = (pc++)->i;
    y2 = xpop();
    y1 = xpop();
    x2 = xpop();
    x1 = xpop();
    dx = x2 - x1;
    if (dx < 1e-10) {
        hoc_execerror("range variable notation r(x1:x2) requires", " x1 > x2");
    }
    sec = nrn_sec_pop();
    di = nrn_section_orientation(sec) ? -1 : 1;
    i2 = node_index(sec, x2) + di;
    i1 = node_index(sec, x1);

    if (s->u.rng.type == VINDEX) {
        if (x1 == 0. || x1 == 1.) {
            nd = node_ptr(sec, x1, (double*) 0);
            if (op) {
                nd->v() = hoc_opasgn(op, NODEV(nd), y1);
            } else {
                nd->v() = y1;
            }
        }
        if (x2 == 1. || x2 == 0.) {
            nd = node_ptr(sec, x2, (double*) 0);
            if (op) {
                nd->v() = hoc_opasgn(op, NODEV(nd), y2);
            } else {
                nd->v() = y2;
            }
        }
        for (i = i1; i != i2; i += di) {
            nd = sec->pnode[i];
            x = ((double) i + .5) / (sec->nnode - 1);
            if (di < 0) {
                x = 1. - x;
            }
            thet = (x - x1) / dx;
            if (thet >= -1e-9 && thet <= 1. + 1e-9) {
                y = y1 * (1. - thet) + y2 * thet;
                if (op) {
                    nd->v() = hoc_opasgn(op, NODEV(nd), y);
                } else {
                    nd->v() = y;
                }
            }
        }
        return;
    }
    if (s->u.rng.type == IMEMFAST) {
        hoc_execerror("i_membrane_ cannot be assigned a value", 0);
    }
    if (s->u.rng.type == MORPHOLOGY) {
        if (!can_change_morph(sec)) {
            return;
        }
        diam_changed = 1;
    }
    indx = range_vec_indx(s);
    for (i = i1; i != i2; i += di) {
        dpr = dprop(s, indx, sec, i);
        x = ((double) i + .5) / (sec->nnode - 1);
        if (di < 0) {
            x = 1. - x;
        }
        thet = (x - x1) / dx;
        if (thet >= -1e-9 && thet <= 1. + 1e-9) {
            y = y1 * (1. - thet) + y2 * thet;
            if (op) {
                *dpr = hoc_opasgn(op, *dpr, y);
            } else {
                *dpr = y;
            }
        }
    }
    if (s->u.rng.type == MORPHOLOGY) {
        sec->recalc_area_ = 1;
        nrn_diam_change(sec);
    }
#if EXTRACELLULAR
    if (s->u.rng.type == EXTRACELL && s->u.rng.index == 0) {
        diam_changed = 1;
    }
    if (s->u.rng.type == EXTRACELL) {
        if (x1 == 0. || x1 == 1.) {
            dpr = neuron::container::data_handle<double>{
                nrn_vext_pd(s, indx, node_ptr(sec, x1, nullptr))};
            if (dpr) {
                if (op) {
                    *dpr = hoc_opasgn(op, *dpr, y1);
                } else {
                    *dpr = y1;
                }
            }
        }
        if (x2 == 1. || x2 == 0.) {
            dpr = neuron::container::data_handle<double>{
                nrn_vext_pd(s, indx, node_ptr(sec, x2, nullptr))};
            if (dpr) {
                if (op) {
                    *dpr = hoc_opasgn(op, *dpr, y2);
                } else {
                    *dpr = y2;
                }
            }
        }
    }
#endif
}

int nrn_exists(Symbol* s, Node* node) {
    if (s->u.rng.type == VINDEX) {
        return 1;
    } else if (nrn_mechanism(s->u.rng.type, node)) {
        return 1;
#if EXTRACELLULAR
    } else if (nrn_vext_pd(s, 0, node)) {
        return 1;
#endif
    } else if (s->u.rng.type == IMEMFAST && nrn_use_fast_imem) {
        return 1;
    } else {
        return 0;
    }
}

neuron::container::data_handle<double> nrn_rangepointer(Section* sec, Symbol* s, double d) {
    /* if you change this change nrnpy_rangepointer as well */
    short i;
    Node* nd;
    int indx;

    if (s->u.rng.type == VINDEX) {
        nd = node_ptr(sec, d, nullptr);
        return nd->v_handle();
    }
    if (s->u.rng.type == IMEMFAST) {
        if (nrn_use_fast_imem) {
            nd = node_ptr(sec, d, nullptr);
            return nd->sav_rhs_handle();
        } else {
            hoc_execerror(
                "cvode.use_fast_imem(1) has not been executed so i_membrane_ does not exist", 0);
        }
    }
    indx = range_vec_indx(s);
#if EXTRACELLULAR
    if (s->u.rng.type == EXTRACELL) {
        double* const pd{nrn_vext_pd(s, indx, node_ptr(sec, d, (double*) 0))};
        if (pd) {
            return neuron::container::data_handle<double>{pd};
        }
    }
#endif
    i = node_index(sec, d);
    return dprop(s, indx, sec, i);
}

/* return nullptr if failure instead of hoc_execerror
   and return pointer to the 0 element if an array
*/
neuron::container::data_handle<double> nrnpy_rangepointer(Section* sec,
                                                          Symbol* s,
                                                          double d,
                                                          int* err,
                                                          int idx) {
    /* if you change this change nrn_rangepointer as well */
    *err = 0;
    if (s->u.rng.type == VINDEX) {
        auto* nd = node_ptr(sec, d, nullptr);
        return nd->v_handle();
    }
    if (s->u.rng.type == IMEMFAST) {
        if (nrn_use_fast_imem) {
            auto* nd = node_ptr(sec, d, nullptr);
            return nd->sav_rhs_handle();
        } else {
            return {};
        }
    }
#if EXTRACELLULAR
    if (s->u.rng.type == EXTRACELL) {
        auto* nd = node_ptr(sec, d, nullptr);
        double* pd{nrn_vext_pd(s, 0, nd)};
        if (pd) {
            return neuron::container::data_handle<double>{pd};
        }
    }
#endif
    auto const i = node_index(sec, d);
    return nrnpy_dprop(s, idx, sec, i, err);
}

// symbol at pc, location on stack, return pointer on stack
void rangevarevalpointer() {
    Symbol* s{(pc++)->sym};
    double d = xpop();
    Section* sec{nrn_sec_pop()};
    if (s->u.rng.type == VINDEX) {
        auto* const nd = node_ptr(sec, d, nullptr);
        hoc_push(nd->v_handle());
        return;
    }
    if (s->u.rng.type == IMEMFAST) {
        if (nrn_use_fast_imem) {
            auto* nd = node_ptr(sec, d, nullptr);
            hoc_push(nd->sav_rhs_handle());
        } else {
            hoc_execerror(
                "cvode.use_fast_imem(1) has not been executed so i_membrane_ does not exist", 0);
        }
        return;
    }
    auto const indx = range_vec_indx(s);
    if (s->u.rng.type == MORPHOLOGY && sec->recalc_area_) {
        nrn_area_ri(sec);
    }
    if (s->u.rng.type == EXTRACELL) {
        double* pd{nrn_vext_pd(s, indx, node_ptr(sec, d, nullptr))};
        if (pd) {
            hoc_pushpx(pd);
            return;
        }
    }
    auto const i = node_index(sec, d);
    hoc_push(dprop(s, indx, sec, i));
}

void rangevareval(void) /* symbol at pc, location on stack, return value on stack */
{
    double* pd;

    rangevarevalpointer();
    pd = hoc_pxpop();
    hoc_pushx(*pd);
}

void rangepoint(void) /* symbol at pc, return value on stack */
{
    hoc_pushx(.5);
    rangevareval();
}

void rangeobjeval(void) /* symbol at pc, section location on stack, return object on stack*/
{
    Symbol* s{(pc++)->sym};
    assert(s->subtype == NMODLRANDOM);  // the only possibility at the moment
    double d = xpop();
    Section* sec{nrn_sec_pop()};
    auto const i = node_index(sec, d);
    Prop* m = nrn_mechanism_check(s->u.rng.type, sec, i);
    Object* ob = nrn_nmodlrandom_wrap(m, s);
    hoc_push_object(ob);
}

void rangeobjevalmiddle(void) /* symbol at pc, return object on stack*/
{
    hoc_pushx(0.5);
    rangeobjeval();
}

int node_index(Section* sec, double x) /* returns nearest index to x */
{
    int i;
    double n;

    if (x < 0. || x > 1.) {
        hoc_execerror("range variable domain is 0<=x<=1", (char*) 0);
    }
    n = (double) (sec->nnode - 1);
    assert(n >= 0.);
    i = n * x;
    if (i == (int) n) {
        i = n - 1;
    }
    if (sec->prop->dparam[3].get<double>()) {
        i = n - i - 1;
    }
    return i;
}

/* return -1 if x at connection end, nnode-1 if at other end */
int node_index_exact(Section* sec, double x) {
    if (x == 0.) {
        if (arc0at0(sec)) {
            return -1;
        } else {
            return sec->nnode - 1;
        }
    } else if (x == 1.) {
        if (arc0at0(sec)) {
            return sec->nnode - 1;
        } else {
            return -1;
        }
    } else {
        return node_index(sec, x);
    }
}
/*USERPROPERTY subtype of VAR used for non range variables associated
with section property
may have special actions (eg. nseg)*/

double cable_prop_eval(Symbol* sym) {
    Section* sec;
    sec = nrn_sec_pop();
    switch (sym->u.rng.type) {
    case 0: /* not in property list so must be nnode */
        return (double) sec->nnode - 1;
    case CABLESECTION:
        return sec->prop->dparam[sym->u.rng.index].get<double>();
    default:
        hoc_execerror(sym->name, " not a USERPROPERTY");
    }
    return 0.;
}
double* cable_prop_eval_pointer(Symbol* sym) {
    Section* sec;
    sec = nrn_sec_pop();
    switch (sym->u.rng.type) {
    case CABLESECTION:
        return &(sec->prop->dparam[sym->u.rng.index].literal_value<double>());
    default:
        hoc_execerror(sym->name, " not a USERPROPERTY that can be pointed to");
    }
    return nullptr;
}

#if KEEP_NSEG_PARM
int keep_nseg_parm_;
void keep_nseg_parm(void) {
    int i = keep_nseg_parm_;
    keep_nseg_parm_ = (int) chkarg(1, 0., 1.);
    hoc_retpushx((double) i);
}
#endif

void nrn_change_nseg(Section* sec, int n) {
    if (n > 32767.) {
        n = 1;
        fprintf(stderr, "requesting %s.nseg=%d but the maximum value is 32767.\n", secname(sec), n);
        hoc_warning("nseg too large, setting to 1.", (char*) 0);
    }
    if (n < 1) {
        hoc_execerror("nseg", " must be positive");
    }
    if (sec->nnode == n + 1) {
        return;
    } else {
        Node** pnd;
        int i;
        int nold = sec->nnode;
        node_alloc(sec, (short) n + 1);
        tree_changed = 1;
        diam_changed = 1;
        sec->recalc_area_ = 1;
        pnd = sec->pnode;
#if KEEP_NSEG_PARM
        if (!keep_nseg_parm_ || nold == 0)
#endif
            for (i = 0; i < n; i++) {
                IGNORE(prop_alloc(&(pnd[i]->prop), MORPHOLOGY, pnd[i]));
                IGNORE(prop_alloc(&(pnd[i]->prop), CAP, pnd[i]));
            }
    }
}
void cable_prop_assign(Symbol* sym, double* pd, int op) {
    Section* sec;
    sec = nrn_sec_pop();
    switch (sym->u.rng.type) {
    case 0: /* not in property list so must be nnode */
        if (op) {
            *pd = hoc_opasgn(op, (double) (sec->nnode - 1), *pd);
        }
        nrn_change_nseg(sec, (int) (*pd));
        break;
    case CABLESECTION:
        if (sym->u.rng.index == 2) {
            if (can_change_morph(sec)) {
                if (op) {
                    *pd = hoc_opasgn(op, sec->prop->dparam[2].get<double>(), *pd);
                }
                sec->prop->dparam[2] = *pd;
                nrn_length_change(sec, *pd);
                diam_changed = 1;
                sec->recalc_area_ = 1;
            }
        } else {
            if (op) {
                *pd = hoc_opasgn(op, sec->prop->dparam[sym->u.rng.index].get<double>(), *pd);
            }
            diam_changed = 1;
            sec->recalc_area_ = 1;
            sec->prop->dparam[sym->u.rng.index] = *pd;
        }
#if RA_WARNING
        if (sym->u.rng.index == 7) {
            ++nrn_ra_set;
        }
#endif
        break;
    default:
        hoc_execerror(sym->name, " not a USERPROPERTY");
    }
}

/* x of parent for this section */
double nrn_connection_position(Section* sec) {
    return sec->prop->dparam[1].get<double>();
}

/* x=0,1 end connected to parent */
double nrn_section_orientation(Section* sec) {
    return sec->prop->dparam[3].get<double>();
}

int nrn_at_beginning(Section* sec) {
    assert(sec->parentsec);
    return nrn_connection_position(sec) == nrn_section_orientation(sec->parentsec);
}

static void nrn_rootnode_alloc(Section* sec) {
    Extnode* nde;
    sec->parentnode = new Node{};
    sec->parentnode->sec_node_index_ = 0;
    sec->parentnode->sec = sec;
#if EXTRACELLULAR
    if (sec->pnode[0]->extnode) {
        prop_alloc(&sec->parentnode->prop, EXTRACELL, sec->parentnode);
        extcell_node_create(sec->parentnode);
    }
#endif
}

Section* nrn_trueparent(Section* sec) {
    Section* psec;
    for (psec = sec->parentsec; psec; psec = psec->parentsec) {
        if (nrn_at_beginning(sec)) {
            sec = psec;
        } else {
            break;
        }
    }
    return psec;
}

void nrn_parent_info(Section* s) {
    /* determine the parentnode using only the authoritative
    info of parentsec
    and arc length connection position */
    /* side effects are that on exit, s will have the right parentnode
       and the true parent (not connected to any section) will have the
       right parentnode
    */

    Section *sec, *psec, *true_parent;
    Node* pnode;
    double x;

    true_parent = (Section*) 0;
    for (sec = s, psec = sec->parentsec; psec; sec = psec, psec = sec->parentsec) {
        if (psec == s) {
            fprintf(stderr, "%s connection to ", secname(s));
            fprintf(stderr, "%s will form a loop\n", secname(s->parentsec));
            nrn_disconnect(s);
            hoc_execerror(secname(s), "connection will form loop");
        }
        x = nrn_connection_position(sec);
        if (x != nrn_section_orientation(psec)) {
            true_parent = psec;
            if (x == 1. || x == 0.) {
                pnode = psec->pnode[psec->nnode - 1];
            } else {
                pnode = psec->pnode[node_index(psec, x)];
            }
            break;
        }
    }
    if (true_parent == (Section*) 0) {
        if (sec->parentnode) {
            /* non nullptr parent node in section without a parent is
                definitely valid
            */
            pnode = sec->parentnode;
        } else {
            nrn_rootnode_alloc(sec);
            pnode = sec->parentnode;
        }
    }
    s->parentnode = pnode;
}

void setup_topology(void) {
    Item* qsec;

    /* use connection info in section property to connect nodes. */
    /* for the moment we assume uniform dx and range 0-1 */

    /* Sections may be connected to a parent at an orientation (usually
        0) at which the node belongs to some ancestor of the parent.
        All difficulties derive from that fact.
        ie. the parentnode may not belong to the parentsec. And
        parent nodes may be invalid. And if they are valid then
        we want to maintain the useful info such as point processes
        which are in them. ie, we only change the parentnode here
        if it is invalid.
    */

    nrn_global_ncell = 0;

    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
#if 0
		if (sec->nnode < 1) { /* last node is not a segment */
			hoc_execerror(secname(sec),
				" has no segments");
		}
#else
        assert(sec->nnode > 0);
#endif
        nrn_parent_info(sec);
        if (!sec->parentsec) {
            ++nrn_global_ncell;
        }
    }

    section_order();
    tree_changed = 0;
    diam_changed = 1;
    v_structure_change = 1;
    ++nrn_shape_changed_;
}

// name of section (for use in error messages)
const char* secname(Section* sec) {
    static char name[512];
    if (sec && sec->prop) {
        if (auto* s = sec->prop->dparam[0].get<Symbol*>(); s) {
            auto indx = sec->prop->dparam[5].get<int>();
            auto* ob = sec->prop->dparam[6].get<Object*>();
            if (ob) {
                Sprintf(name,
                        "%s.%s%s",
                        hoc_object_name(ob),
                        s->name,
                        hoc_araystr(s, indx, ob->u.dataspace));
            } else {
                Sprintf(name, "%s%s", s->name, hoc_araystr(s, indx, hoc_top_level_data));
            }
#if USE_PYTHON
        } else if (sec->prop->dparam[PROP_PY_INDEX].get<void*>()) {
            assert(nrnpy_pysec_name_p_);
            return (*nrnpy_pysec_name_p_)(sec);
#endif
        } else {
            name[0] = '\0';
        }
    } else {
        name[0] = '\0';
    }
    return name;
}

const char* nrn_sec2pysecname(Section* sec) {
#if USE_PYTHON
    static char buf[256];
    const char* name = secname(sec);
    if (sec && sec->prop->dparam[PROP_PY_INDEX].get<void*>() &&
        strncmp(name, "__nrnsec_0x", 11) != 0) {
        Sprintf(buf, "_pysec.%s", name);
    } else {
        strcpy(buf, name);
    }
    return buf;
#else
    return secname(sec);
#endif
}

void section_owner(void) {
    Section* sec;
    Object* ob;
    sec = chk_access();
    ob = nrn_sec2cell(sec);
    hoc_ret();
    hoc_push_object(ob);
}

char* hoc_section_pathname(Section* sec) {
    static char name[200];
    if (auto* s = sec->prop->dparam[0].get<Symbol*>(); sec && sec->prop && s) {
        auto indx = sec->prop->dparam[5].get<int>();
        auto* ob = sec->prop->dparam[6].get<Object*>();
        if (ob) {
            char* p = hoc_object_pathname(ob);
            if (p) {
                Sprintf(name, "%s.%s%s", p, s->name, hoc_araystr(s, indx, ob->u.dataspace));
            } else {
                hoc_warning("Can't find a pathname for", secname(sec));
                strcpy(name, secname(sec));
                return name;
            }
        } else {
            Sprintf(name, "%s%s", s->name, hoc_araystr(s, indx, hoc_objectdata));
        }
#if USE_PYTHON
    } else if (sec && sec->prop && sec->prop->dparam[PROP_PY_INDEX].get<void*>()) {
        strcpy(name, nrn_sec2pysecname(sec));
#endif
    } else {
        name[0] = '\0';
    }
    return name;
}

double nrn_arc_position(Section* sec, Node* node) {
    int inode;
    double x;
    assert(sec);
    if (sec->parentnode == node) {
        x = 0.;
    } else if ((inode = node->sec_node_index_) == sec->nnode - 1) {
        x = 1.;
    } else {
        x = ((double) inode + .5) / ((double) sec->nnode - 1.);
    }
    if (arc0at0(sec)) {
        return x;
    } else {
        return 1. - x;
    }
}

const char* sec_and_position(Section* sec, Node* nd) {
    const char* buf;
    static char buf1[200];
    double x;
    assert(sec);
    buf = secname(sec);
    x = nrn_arc_position(sec, nd);
    Sprintf(buf1, "%s(%g)", buf, x);
    return buf1;
}

int segment_limits(double* pdx) {
    int n;
    Section* sec;
    double l;

    sec = chk_access();
    n = sec->nnode - 1;
    /*	l = sec->prop->dparam[2].val;*/
    l = 1.;
    *pdx = l / ((double) n);
    return sec->nnode;
}

#undef PI
#define PI 3.14159265358979323846

Node* node_exact(Section* sec, double x) {
    /* like node_index but give proper node when
        x is 0 or 1 as well as in between
    */
    Node* node;
    assert(sec);
    {
        if (x <= 0. || x >= 1.) {
            x = (x < 0.) ? 0. : x;
            x = (x > 1.) ? 1. : x;
            if (sec->prop->dparam[3].get<double>()) {
                x = 1. - x;
            }
            if (x == 0.) {
                if (tree_changed) {
                    setup_topology();
                }
                node = sec->parentnode;
            } else {
                node = sec->pnode[sec->nnode - 1];
            }
        } else {
            node = sec->pnode[node_index(sec, x)];
        }
    }
    return node;
}

Node* node_ptr(Section* sec, double x, double* parea) {
    /* returns pointer to proper node and the area of the node */
    Node* nd;

    nd = node_exact(sec, x);
    if (parea) {
        if (nd->sec->recalc_area_) {
            nrn_area_ri(nd->sec);
        }
        *parea = NODEAREA(nd);
    }
    return nd;
}

int nrn_get_mechtype(const char* mechname) {
    Symbol* s;
    s = hoc_lookup(mechname);
    assert(s);
    if (s->type == TEMPLATE) {
        s = hoc_table_lookup(mechname, s->u.ctemplate->symtable);
        assert(s && s->type == MECHANISM);
    }
    return s->subtype;
}

int nrn_instance_count(int mechtype) {
    if (v_structure_change) {
        v_setup_vectors();
    }
    return memb_list[mechtype].nodecount;
}

#if EXTRACELLULAR
/* want to handle vext(0), vext(1) correctly. No associated i_membrane though.*/
/*
otherwise return correct pointer to vext if it exists
return pointer to zero if a child node has extnode
return 0 if symbol is not vext.
*/
double* nrn_vext_pd(Symbol* s, int indx, Node* nd) {
    static double zero;
    if (s->u.rng.type != EXTRACELL) {
        return (double*) 0;
    }
    if (s->u.rng.index != neuron::extracellular::vext_pseudoindex()) {
        return nullptr;
    }
    zero = 0.;
    if (nd->extnode) {
        return nd->extnode->v + indx;
    } else {
        Section* sec;
        /* apparently not maintaining Node.child */
        /*for (sec = nd->child; sec; sec = sec->sibling) {*/
        for (sec = nd->sec->child; sec; sec = sec->sibling) {
            if (sec->pnode[0]->extnode) {
                return &zero;
            }
        }
        return (double*) 0;
    }
}
#endif

/* if you change this then change nrnpy_dprop as well */
/* returns location of property symbol */
neuron::container::data_handle<double> dprop(Symbol* s, int indx, Section* sec, short inode) {
    auto* const m = nrn_mechanism_check(s->u.rng.type, sec, inode);
#if EXTRACELLULAR
    // old comment: this does not handle vext(0) and vext(1) properly at this time
    if (m->_type == EXTRACELL && s->u.rng.index == neuron::extracellular::vext_pseudoindex()) {
        return neuron::container::data_handle<double>{neuron::container::do_not_search,
                                                      sec->pnode[inode]->extnode->v + indx};
    }
#endif
    if (s->subtype != NRNPOINTER) {
        if (m->ob) {
            return neuron::container::data_handle<double>{m->ob->u.dataspace[s->u.rng.index].pval +
                                                          indx};
        } else {
            return m->param_handle_legacy(s->u.rng.index + indx);
        }
    } else {
        neuron::container::data_handle<double> const p{m->dparam[s->u.rng.index + indx]};
        if (!p) {
            hoc_execerror(s->name, "wasn't made to point to anything");
        }
        return p;
    }
}

/* return nullptr instead of hoc_execerror. */
/* returns location of property symbol */
neuron::container::data_handle<double> nrnpy_dprop(Symbol* s,
                                                   int indx,
                                                   Section* sec,
                                                   short inode,
                                                   int* err) {
    auto* const m = nrn_mechanism(s->u.rng.type, sec->pnode[inode]);
    if (!m) {
        *err = 1;
        return {};
    }
#if EXTRACELLULAR
    /* this does not handle vext(0) and vext(1) properly at this time */
    if (m->_type == EXTRACELL && s->u.rng.index == neuron::extracellular::vext_pseudoindex()) {
        return neuron::container::data_handle<double>{sec->pnode[inode]->extnode->v + indx};
    }
#endif
    if (s->subtype != NRNPOINTER) {
        if (m->ob) {
            return neuron::container::data_handle<double>{m->ob->u.dataspace[s->u.rng.index].pval +
                                                          indx};
        } else {
            return m->param_handle_legacy(s->u.rng.index + indx);
        }
    } else {
        neuron::container::data_handle<double> const p{m->dparam[s->u.rng.index + indx]};
        if (!p) {
            *err = 2;
        }
        return p;
    }
}

static char* objectname(void) {
    static char buf[100];
    if (hoc_thisobject) {
        Sprintf(buf, "%s", hoc_object_name(hoc_thisobject));
    } else {
        buf[0] = '\0';
    }
    return buf;
}

#define relative(pc) (pc + (pc)->i)

void forall_section(void) {
    /*statement pointed to by pc
    continuation pointed to by pc+1. template used is shortfor in code.cpp
    of hoc system.
    */
    /* if inside object then forall refers only to sections in the object */

    Inst* savepc = pc;
    Item *qsec, *first, *last;
    extern int hoc_returning;
    char buf[200];
    char** s;
    int istk;

    /* fast forall within an object asserts that the object sections
       are contiguous and secelm_ is the last.
    */
    if (hoc_thisobject) {
        qsec = hoc_thisobject->secelm_;
        if (qsec) {
            for (first = qsec;
                 first->prev->itemtype &&
                 hocSEC(first->prev)->prop->dparam[6].get<Object*>() == hoc_thisobject;
                 first = first->prev) {
            }
            last = qsec->next;
        } else {
            first = (Item*) 0;
            last = (Item*) 0;
        }
    } else {
        first = section_list->next;
        last = section_list;
    }
    s = hoc_strpop();
    buf[0] = '\0';
    if (s) {
        Sprintf(buf, "%s.*%s.*", objectname(), *s);
    } else {
        char* o = objectname();
        if (o[0]) {
            Sprintf(buf, "%s.*", o);
        }
    }
    istk = nrn_isecstack();
    /* do the iteration safely. a possible command is to delete the section*/
    for (qsec = first; qsec != last;) {
        Section* sec = hocSEC(qsec);
        qsec = qsec->next;
        if (buf[0]) {
            std::regex pattern(escape_bracket(buf));
            if (!std::regex_match(secname(sec), pattern)) {
                continue;
            }
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
    if (!hoc_returning)
        pc = relative(savepc + 1);
}

void hoc_ifsec(void) {
    Inst* savepc = pc;
    char buf[200];
    char** s;
    extern int hoc_returning;

    s = hoc_strpop();
    Sprintf(buf, ".*%s.*", *s);
    std::regex pattern(escape_bracket(buf));
    if (std::regex_match(secname(chk_access()), pattern)) {
        hoc_execute(relative(savepc));
    }
    if (!hoc_returning)
        pc = relative(savepc + 1);
}

void issection(void) { /* returns true if string is the access section */
    std::regex pattern(escape_bracket(gargstr(1)));
    if (std::regex_match(secname(chk_access()), pattern)) {
        hoc_retpushx(1.);
    } else {
        hoc_retpushx(0.);
    }
}

int has_membrane(char* mechanism_name, Section* sec) {
    /* return true if string is an inserted membrane in the
        section sec */
    Prop* p;
    for (p = sec->pnode[0]->prop; p; p = p->next) {
        if (strcmp(memb_func[p->_type].sym->name, mechanism_name) == 0) {
            return (1);
        }
    }
    return (0);
}

void ismembrane(void) { /* return true if string is an inserted membrane in the
        access section */
    char* str;
    Prop* p;

    str = gargstr(1);
    hoc_retpushx((double) has_membrane(str, chk_access()));
}

const char* secaccessname(void) {
    return secname(chk_access());
}

void sectionname(void) {
    char** cpp;

    cpp = hoc_pgargstr(1);
    if (ifarg(2) && chkarg(2, 0., 1.) == 0.) {
        hoc_assign_str(cpp, secname(chk_access()));
    } else {
        hoc_assign_str(cpp, nrn_sec2pysecname(chk_access()));
    }
    hoc_retpushx(1.);
}

void hoc_secname(void) {
    static char* buf = (char*) 0;
    Section* sec = chk_access();
    if (!buf) {
        buf = static_cast<char*>(emalloc(256 * sizeof(char)));
    }
    if (ifarg(1) && chkarg(1, 0., 1.) == 0.) {
        strcpy(buf, secname(sec));
    } else {
        strcpy(buf, nrn_sec2pysecname(sec));
    }
    hoc_ret();
    hoc_pushstr(&buf);
}

static double chk_void2dbl(void* vp, const char* mes) {
    size_t n = (size_t) vp;
    if (sizeof(void*) == 8) {
        size_t maxvoid2dbl = ((size_t) 1) << 53;
#if 0
    printf("sizeof void*, size_t, and double = %zd, %zd, %zd\n",
      sizeof(void*), sizeof(size_t), sizeof(double));
    printf("(double)((1<<53) - 1)=%.20g\n", (double)(maxvoid2dbl - 1));
    printf("(double)((1<<53) + 0)=%.20g\n", (double)(maxvoid2dbl));
    printf("(double)((1<<53) + 1)=%.20g\n", (double)(maxvoid2dbl + 1));
    printf("(size_t)(vp) = %zd\n", n);
#endif
        if (n > maxvoid2dbl) {
            hoc_execerror(mes, "pointer too large to be represented by a double");
        }
    } else if (sizeof(void*) > 8) {
        hoc_execerror(mes, "not implemented for sizeof(void*) > 8");
    }
    return (double) n;
}

void this_section(void) {
    /* return section number of currently accessed section at
        arc length postition x */

    Section* sec;
    sec = chk_access();
    hoc_retpushx(chk_void2dbl(sec, "this_section"));
}
void this_node(void) {
    /* return node number of currently accessed section at
        arc length postition x */

    Section* sec;
    Node* nd;
    sec = chk_access();
    nd = node_exact(sec, *getarg(1));
    hoc_retpushx(chk_void2dbl(nd, "this_node"));
}
void parent_section(void) {
    /* return section number of currently accessed section at
        arc length postition x */

    Section* sec;
    sec = chk_access();
    hoc_retpushx(chk_void2dbl(sec->parentsec, "parent_section"));
}
void parent_connection(void) {
    Section* sec;
    sec = chk_access();
    hoc_retpushx(nrn_connection_position(sec));
}

void section_orientation(void) {
    Section* sec;
    sec = chk_access();
    hoc_retpushx(nrn_section_orientation(sec));
}

void parent_node(void) {
    Section* sec;
    if (tree_changed) {
        setup_topology();
    }
    sec = chk_access();
    hoc_retpushx(chk_void2dbl(sec->parentnode, "parent_node"));
}

void pop_section(void) {
    --skip_secstack_check;
    if (skip_secstack_check < 0) {
        skip_secstack_check = 0;
    }
    nrn_popsec();
    hoc_retpushx(1.);
}

/* turn off section stack fixing (in case of return,continue,break in a section
statement) between exlicit user level push_section,etc and pop_section
*/

void hoc_level_pushsec(Section* sec) {
    ++skip_secstack_check;
    nrn_pushsec(sec);
}

void push_section(void) {
    Section* sec;
    if (hoc_is_str_arg(1)) {
        Item* qsec;
        char* s;
        sec = (Section*) 0;
        s = gargstr(1);
        // ForAllSections(sec1) /* I can't imagine a more inefficient way */
        ITERATE(qsec, section_list) {
            Section* sec1 = hocSEC(qsec);
            if (strcmp(s, nrn_sec2pysecname(sec1)) == 0) {
                sec = sec1;
                break;
            }
        }
        if (!sec) {
            hoc_execerror("push_section: arg not a sectionname:", s);
        }
    } else {
        sec = (Section*) (size_t) (*getarg(1));
    }
    if (!sec || !sec->prop || !sec->prop->dparam || !sec->prop->dparam[8].get<hoc_Item*>() ||
        sec->prop->dparam[8].get<hoc_Item*>()->itemtype != SECTION) {
        hoc_execerror("Not a Section pointer", (char*) 0);
    }
    hoc_level_pushsec(sec);
    hoc_retpushx(1.0);
}


Section* nrn_section_exists(char* name, int indx, Object* cell) {
    Section* sec = (Section*) 0;
    Symbol* sym = (Symbol*) 0;
    Object* obj = cell;
    Objectdata* obd;
    Item* itm;
    if (obj) {
        sym = hoc_table_lookup(name, obj->ctemplate->symtable);
        /* if external then back to top level */
        if (sym && sym->cpublic == 2) {
            sym = sym->u.sym;
            obj = nullptr;
        }
    } else {
        sym = hoc_table_lookup(name, hoc_top_level_symlist);
    }
    if (sym) {
        if (sym->type == SECTION) {
            if (obj) {
                obd = obj->u.dataspace;
            } else {
                obd = hoc_top_level_data;
            }
            if (indx < hoc_total_array_data(sym, obd)) {
                itm = *(obd[sym->u.oboff].psecitm + indx);
                if (itm) {
                    sec = itm->element.sec;
                }
            }
        }
    }
    return sec;
}

void section_exists(void) {
    int iarg, indx;
    Section* sec;
    Object* obj;
    char *str, *cp, buf[100];

    obj = nullptr;
    sec = (Section*) 0;
    iarg = 1;
    str = gargstr(iarg++);

    indx = 0;
    if (ifarg(iarg) && hoc_is_double_arg(iarg)) {
        indx = (int) chkarg(iarg++, 0., 1e9);
    } else { /* if [integer] present, then extract the value and the basename */
        if (sscanf(str, "%[^[][%d", buf, &indx) == 2) {
            str = buf;
        }
    }
    if (ifarg(iarg)) {
        obj = *hoc_objgetarg(iarg);
    }
    sec = nrn_section_exists(str, indx, obj);
    hoc_retpushx((double) (sec && sec->prop));
}

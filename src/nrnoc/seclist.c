#include <../../nrnconf.h>
#define HOC_L_LIST 1
#include "section.h"
#include "neuron.h"
#include "parse.h"
#include "hocparse.h"
#include "code.h"
#include "hoc_membf.h"

/*ARGSUSED*/
static void* constructor(Object* ho)
{
	List* sl;
	sl = newlist();	
	return (void*)sl;
}

static void destructor(void* v)
{
	Item* q;
	List* sl = (List*)v;
	ITERATE(q, sl) {
		section_unref(q->element.sec);
	}
	freelist(&sl);
}

static double append(void* v)
{
	Section* sec = chk_access();
	lappendsec((List*)v, sec);
	section_ref(sec);
	return 1.;
}


static Item* children1(List* sl, Section* sec)
{
	Item* i;
	Section* ch;
	i = sl->prev;
	for (ch = sec->child; ch; ch = ch->sibling) {
		i = lappendsec(sl, ch);
		section_ref(ch);
	}
	return i;
}

static double children(void* v)
{
	Section* sec;
	List* sl;
	sec = chk_access();
	sl = (List*)v;
	children1(sl, sec);
	return 1.;
}

static Item* subtree1(List* sl, Section* sec)
{
	Item* i, *j, *last, *first;
	Section* s;
	/* it is confusing to span the tree from the root without using
	  recursion.
	*/
	s = sec;
	i = lappendsec(sl, s);
	section_ref(s);
	last = i->prev;
	while( i != last) {
		for (first = last->next, last = i, j = first; j->prev != last; j = j->next) {
			s = hocSEC(j);
			i = children1(sl, s);
		}
	}
	return i;
}

static double subtree(void* v)
{
	Section* sec;
	List* sl;
	sec = chk_access();
	sl = (List*)v;
	subtree1(sl, sec);
	return 1.;
}

static double wholetree(void* v)
{
	List* sl;
	Section* s, *sec, *ch;
	Item* i, *j, *first, *last;
	sec = chk_access();
	sl = (List*)v;
	/*find root*/
	for (s = sec; s->parentsec; s = s->parentsec) {}

	subtree1(sl, s);
	return 1.;
}

static double allroots(void* v)
{
	List* sl;
	Item* qsec;
	sl = (List*)v;
	ForAllSections(sec)
		if (!sec->parentsec) {
			lappendsec(sl, sec);
			section_ref(sec);
		}
	}

	return 1.;
}

static double seclist_remove(void* v)
{
	Section* sec, *s;
	Item* q, *q1;
	List* sl;
	int i;

	sl = (List*)v;
	i = 0;
    if (!ifarg(1)) {
	sec = chk_access();
	ITERATE(q, sl) {
		if (sec == q->element.sec) {
			delete(q);
			section_unref(sec);
			return 1.;
		}
	}
	hoc_warning(secname(sec), "not in this section list");
    }else{
	Object* o;
	o = *hoc_objgetarg(1);
	check_obj_type(o, "SectionList");
	ITERATE(q, sl) {
		s = hocSEC(q);
		s->volatile_mark = 0;
	}
	sl = (List*)o->u.this_pointer;
	ITERATE(q, sl) {
		s = hocSEC(q);
		s->volatile_mark = 1;
	}
	sl = (List*)v;
	i = 0;
	for (q = sl->next; q != sl; q = q1) {
		q1 = q->next;
		s = hocSEC(q);
		if (s->volatile_mark) {
			delete(q);
			section_unref(s);
			++i;
		}
	}
    }
	return (double) i;
}

static double unique(void* v)
{
	int i; /* number deleted */
	Section* s;
	Item* q, *q1;
	List* sl = (List*)v;
	ITERATE(q, sl) {
		s = hocSEC(q);
		s->volatile_mark = 0;
	}
	i = 0;
	for (q = sl->next; q != sl; q = q1) {
		q1 = q->next;
		s = hocSEC(q);
		if (s->volatile_mark++) {
			delete(q);
			section_unref(s);
			++i;
		}
	}
	return (double)i;
}

static double contains(void* v)
{
	Section* s;
	Item* q;
	List* sl = (List*)v;
	s = chk_access();
	ITERATE(q, sl) {
		if (hocSEC(q) == s) {
			return 1.;
		}
	}
	return (0.);
}

static double printnames(void* v)
{
	Item* q;
	List* sl = (List*)v;
	ITERATE(q, sl) {
		printf("%s\n", secname(q->element.sec));
	}
	return 1.;
}

static Member_func members[] = {
	"append", append,
	"remove", seclist_remove,
	"wholetree", wholetree,
	"subtree", subtree,
	"children", children,
	"unique", unique,
	"printnames", printnames,
	"contains", contains,
	"allroots", allroots,
	0,0
};

void SectionList_reg(void) {
	void class2oc();
/*	printf("SectionList_reg\n");*/
	class2oc("SectionList", constructor, destructor, members, (void*)0, (void*)0, (void*)0);
}

#define relative(pc)	(pc + (pc)->i)
extern int hoc_returning;

static void check(Object* ob) {
	if (!ob) {
		hoc_execerror("nil object is not a SectionList", (char*)0);
	}
	if (ob->template->constructor != constructor) {
		hoc_execerror(ob->template->sym->name, " is not a SectionList");
	}
}

void forall_sectionlist(void) {
	Inst* savepc = pc;
	Item* q, *q1;
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
	sl = (List*)(ob->u.this_pointer);
	istk = nrn_isecstack();
	for (q = sl->next; q != sl; q = q1) {
		q1 = q->next;
		sec = q->element.sec;
		if (!sec->prop) {
			delete(q);
			section_unref(sec);
			continue;
		}
		nrn_pushsec(sec);
		hoc_execute(relative(savepc));
		nrn_popsec();
		if (hoc_returning) { nrn_secstack(istk); }
		if (hoc_returning == 1 || hoc_returning == 4) {
			break;
		}else if (hoc_returning ==2) {
			hoc_returning = 0;
			break;
		}else{
			hoc_returning = 0;
		}
	}
	hoc_tobj_unref(obp);
	if (!hoc_returning) {
		pc = relative(savepc+1);
	}
}

void hoc_ifseclist(void) {
	Inst *savepc = pc;
	Item* q;
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
	sl = (List*)(ob->u.this_pointer);
	ITERATE(q, sl) {
		if (sec == q->element.sec) {
			hoc_execute(relative(savepc));
			if (!hoc_returning) {
				pc = relative(savepc+1);
			}
			hoc_tobj_unref(obp);
			return;
		}
	}
	hoc_tobj_unref(obp);
	if (!hoc_returning) {
		pc = relative(savepc+1);
	}
}


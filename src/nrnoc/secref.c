#include <../../nrnconf.h>
/*
Section reference
Allows sections to be stored as variables and passed as arguments by
using an object as a label for a particular section.

Usage is:

objectvar s1, s2
soma s1 = new SectionRef()
print s1.sec.L	// prints length of soma
s2 = s1
s2.sec {psection()} // makes soma the currently accessed section for a statement

access s1.sec	// soma becomes the default section
*/

#include <stdlib.h>
#include "section.h"
#include "parse.h"
#include "hoc_membf.h"

Symbol* nrn_sec_sym, *nrn_parent_sym, *nrn_root_sym, *nrn_child_sym;
Symbol* nrn_trueparent_sym;

static hoc_Item** sec2pitm(Section* sec) {
	extern Objectdata* hoc_top_level_data;
	Symbol* sym;
	Object* ob;
	int i;
	if (!sec || !sec->prop || !sec->prop->dparam[0].sym) {
		hoc_execerror("section is unnamed", (char*)0);
	}
	sym = sec->prop->dparam[0].sym;
	ob = sec->prop->dparam[6].obj;
	i = sec->prop->dparam[5].i;
	if (ob) {
		return ob->u.dataspace[sym->u.oboff].psecitm + i;
	}else{
		return hoc_top_level_data[sym->u.oboff].psecitm + i;
	}
}

/*ARGSUSED*/
static void* cons(Object* ho) {
	Section* sec = chk_access();
	section_ref(sec);
	return (void*)(sec);
}

static void destruct(void* v) {
	Section* sec = (Section*)v;
	section_unref(sec);
}

#if 0
static double dummy(void* v) {
	Section* sec = (Section*)v;
	printf("%s\n", secname(sec));
	return 0.;
}
#endif

static double s_unname(void* v) {
	hoc_Item** pitm;
	Section* sec;
	sec = (Section*)v;
	pitm = sec2pitm(sec);
	*pitm = (hoc_Item*)0;
	sec->prop->dparam[0].sym = (Symbol*)0;
	return 1.;
}

static double s_rename(void* v) {
	extern Object* hoc_thisobject, **hoc_objgetarg();
	extern Objectdata* hoc_top_level_data;
	extern Symbol* hoc_table_lookup();
	extern Symlist* hoc_top_level_symlist;
	extern Object* ivoc_list_item();
	char* name;
	Section* sec;
	Symbol* sym;
	hoc_Item* qsec;
	hoc_Item** pitm;
	Object* ob, *olist;
	Objectdata* obdsav;
	int i, index, size, n;

	sec = (Section*)v;
	if (!sec->prop) {
		printf("SectionRef[???].sec is a deleted section\n");
		return 0.;
	}
	qsec = sec->prop->dparam[8].itm;
	if (sec->prop->dparam[0].sym) {
		printf("%s must first be unnamed\n", secname(sec));
		return 0.;
	}

	name = gargstr(1);
	size = 0;
	index = 0;
	if (ifarg(2)) { 
		olist = *hoc_objgetarg(2);
		size = ivoc_list_count(olist);
		assert(size > 0);
	}
	sym = hoc_table_lookup(name, hoc_top_level_symlist);
	obdsav = hoc_objectdata;
	hoc_objectdata = hoc_top_level_data;
	if (sym) {
		if (sym->type != SECTION || (sym->arayinfo && sym->arayinfo->nsub > 1)) {
			printf("The new name already exists and is not a SECTION or has a dimension > 1\n");
			hoc_objectdata = obdsav;
			return 0;
		}
		/* check that it points to no sections */
		n = hoc_total_array_data(sym, hoc_top_level_data);
		pitm = hoc_top_level_data[sym->u.oboff].psecitm;
		for (i=0; i < n; ++i) {
			if (pitm[i]) {
				printf("Previously existing %s[%d] points to a section which is being deleted\n", sym->name, i);
				sec_free(pitm[i]);
			}
		}
		if (sym->arayinfo) {
			hoc_freearay(sym);
		}			
		free(pitm);
	}else{
		sym = hoc_install(name, SECTION, 0., &hoc_top_level_symlist);
		hoc_install_object_data_index(sym);
	}
	if (size) {
		hoc_pushx((double)size);
		hoc_arayinfo_install(sym, 1);
		hoc_top_level_data[sym->u.oboff].psecitm = pitm = (hoc_Item**)ecalloc(size, sizeof(hoc_Item*));
	}else{
		hoc_top_level_data[sym->u.oboff].psecitm = pitm = (hoc_Item**)ecalloc(1, sizeof(hoc_Item*));
	}

	if (size == 0) {
		pitm[index] = qsec;
		sec->prop->dparam[0].sym = sym;
		sec->prop->dparam[5].i = index;
		sec->prop->dparam[6].obj = (Object*)0;
		OPSECITM(sym)[0] = qsec;
	}else{
		for (i=0; i < size; ++i) {
			ob = ivoc_list_item(olist, i);
			/*assert(is_obj_type(ob, "SectionRef")*/
			sec = (Section*)ob->u.this_pointer;
			if (!sec->prop) {
	printf("%s references a deleted section\n", hoc_object_name(ob));
				hoc_objectdata = obdsav;
				return 0;
			}
			qsec = sec->prop->dparam[8].itm;
			sec->prop->dparam[0].sym = sym;
			sec->prop->dparam[5].i = i;
			sec->prop->dparam[6].obj = (Object*)0;
			OPSECITM(sym)[i] = qsec;
		}
	}
#if 0
	printf("SectionRef[???}.rename");
	i = sec->prop->dparam[5].i;
	if (ob) {
		pitm = ob->u.dataspace[sym->u.oboff].psecitm;
	}else{
		pitm = hoc_top_level_data[sym->u.oboff].psecitm;
	}
	printf("%s\n", secname(sec));
	printf("sec->prop->dparam[0].sym->name = %s\n", sym->name);
	printf("dparam[5].i = %d  dparam[6].obj = %s\n", i, hoc_object_name(ob));
	printf("sym->u.oboff = %d\n", sym->u.oboff);
	if (ob) {
		printf("ob->u.dataspace[sym->u.oboff].psecitm = %lx\n", pitm);
	}else{
		printf("hoc_top_level_data[sym->u.oboff].psecitm = %lx\n", pitm);
	}
	printf("hocSEC(pitm[i]) = %s\n", secname(hocSEC(pitm[i])));
	if (sym->arayinfo) {
		Arrayinfo* a;
		int j;
		a = sym->arayinfo;
		printf("symbol array info ");
		for (j=0; j < a->nsub; ++j) {
			printf("[%d]", a->sub[j]);
		}
		printf("\n");
		if (ob) {
			a = ob->u.dataspace[sym->u.oboff + 1].arayinfo;
			printf("dataspace array info ");
			for (j=0; j < a->nsub; ++j) {
				printf("[%d]", a->sub[j]);
			}
			printf("\n");
		}
	}else{
		printf("scalar\n");
	}
#endif
	hoc_objectdata = obdsav;
	return 1;
}

int nrn_secref_nchild(Section* sec) {
	int n;
	if (!sec->prop) {
		hoc_execerror("Section was deleted", (char*)0);
	}
	for (n=0, sec = sec->child; sec; sec = sec->sibling) {
		++n;
	}
	return n;
}

static double s_nchild(void* v) {
	int n;
	return (double)nrn_secref_nchild((Section*)v);
}

static double s_has_parent(void* v) {
	int n;
	Section* sec = (Section*)v;
	if (!sec->prop) {
		hoc_execerror("Section was deleted", (char*)0);
	}
	return (double)(sec->parentsec != (Section*)0);
}

static double s_has_trueparent(void* v) {
	int n;
	Section* sec = (Section*)v;
	if (!sec->prop) {
		hoc_execerror("Section was deleted", (char*)0);
	}
	return (double)(nrn_trueparent(sec) != (Section*)0);
}

static double s_exists(void* v) {
	int n;
	Section* sec = (Section*)v;
	return (double)(sec->prop != (Prop*)0);
}

static double s_cas(void* v) { /* return 1 if currently accessed section */
	Section* sec = (Section*)v;
	Section* cas = chk_access();
	if (!sec->prop) {
		hoc_execerror("Section was deleted", (char*)0);
	}
	if (sec == cas) {
		return 1.;
	}
	return 0.;
}

static Member_func members[] = {
	"sec", s_rename,	/* will actually become a SECTIONREF below */
	"parent", s_rename,
	"trueparent", s_rename,
	"root", s_rename,
	"child", s_rename,
	"nchild", s_nchild,
	"has_parent", s_has_parent,
	"has_trueparent", s_has_trueparent,
	"exists", s_exists,
	"rename", s_rename,
	"unname", s_unname,
	"is_cas", s_cas,
	0, 0
};

Section* nrn_sectionref_steer(Section* sec, Symbol* sym, int* pnindex)
{
	Section* s;
	if (sym == nrn_parent_sym) {
		s = sec->parentsec;
		if (!s) {
			hoc_execerror("SectionRef has no parent for ", secname(sec));
		}
	}else if (sym == nrn_trueparent_sym) {
		s = nrn_trueparent(sec);
		if (!s) {
			hoc_execerror("SectionRef has no parent for ", secname(sec));
		}
	}else if (sym == nrn_root_sym) {
		for (s = sec; s->parentsec; s = s->parentsec) {}
	}else if (sym == nrn_child_sym) {
		int index, i;
		if (*pnindex == 0) {
			hoc_execerror("SectionRef.child[index]", (char*)0);
		}
		index = (int)hoc_xpop();
		--*pnindex;
		for (i=0, s = sec->child; i < index && s; s = s->sibling) {
			++i;
		}
		if (i != index || !s) {
			hoc_execerror("SectionRef.child index too large for", secname(sec));
		}
	}
	return s;
}

void SectionRef_reg(void) {
	Symbol* s, *sr, *hoc_table_lookup();
	extern void class2oc();
	
	class2oc("SectionRef", cons, destruct, members, (void*)0, (void*)0, (void*)0);
	/* now make the sec variable an actual SECTIONREF */
	sr = hoc_lookup("SectionRef");
	s = hoc_table_lookup("sec", sr->u.template->symtable);
	s->type = SECTIONREF;
	nrn_sec_sym = s;
	s = hoc_table_lookup("parent", sr->u.template->symtable);
	s->type = SECTIONREF;
	nrn_parent_sym = s;
	s = hoc_table_lookup("trueparent", sr->u.template->symtable);
	s->type = SECTIONREF;
	nrn_trueparent_sym = s;
	s = hoc_table_lookup("root", sr->u.template->symtable);
	s->type = SECTIONREF;
	nrn_root_sym = s;
	s = hoc_table_lookup("child", sr->u.template->symtable);
	s->type = SECTIONREF;
	nrn_child_sym = s;
	s->arayinfo = (Arrayinfo*)emalloc(sizeof(Arrayinfo));;
	s->arayinfo->refcount = 1;
	s->arayinfo->a_varn = (void*)0;
	s->arayinfo->nsub = 1;
	s->arayinfo->sub[0] = 0;
}

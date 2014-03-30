#include <../../nrnconf.h>
#include <OS/string.h>
#include <InterViews/regexp.h>
#include <stdio.h>
#include <stdlib.h>
#include "classreg.h"
#include "oc2iv.h"
#include <string.h>
// for alias
#include <symdir.h>
#include <oclist.h>
#include <parse.h>
// for references
#include <hoclist.h>
#if HAVE_IV
#include <ocbox.h>
#endif
extern "C" {
extern Objectdata* hoc_top_level_data;
extern Symlist* hoc_built_in_symlist;
extern int nrn_is_artificial(int);
}

inline unsigned long key_to_hash(String& s) {return s.hash();}
implementTable(SymbolTable, String, Symbol*)

static double l_substr(void*) {
	char* s1 = gargstr(1);
	char* s2 = gargstr(2);
	char* p = strstr(s1, s2);
	if (p) {
		return double(p - s1);
	}else{
		return -1.;
	}
}

static double l_len(void*) {
	return double(strlen(gargstr(1)));
}

static double l_head(void*) {
	String text(gargstr(1));
	Regexp r(gargstr(2));
	r.Search(text.string(), text.length(), 0, text.length());
	int i = r.BeginningOfMatch();
//	text.set_to_left(i); doesnt work
	char** head = hoc_pgargstr(3);
	if (i > 0) {
		char* buf = new char[i+1];
		strncpy(buf, text.string(), i);
		buf[i] = '\0';
		hoc_assign_str(head, buf);
		delete [] buf;
	}else{
		hoc_assign_str(head, "");
	}
	return double(i);
}

static double l_tail(void*) {
	CopyString text(gargstr(1));
	Regexp r(gargstr(2));
	r.Search(text.string(), text.length(), 0, text.length());
	int i = r.EndOfMatch();
	char** tail = hoc_pgargstr(3);
	if (i >= 0) {
		hoc_assign_str(tail, text.string() + i);
	}else{
		hoc_assign_str(tail, "");
	}
	return double(i);
}

static double l_left(void*) {
	CopyString text(gargstr(1));
	CopyString newtext = text.left(int(chkarg(2,0,strlen(gargstr(1)))));
	hoc_assign_str(hoc_pgargstr(1), newtext.string());
	return 1.;
}

static double l_right(void*) {
	CopyString text(gargstr(1));
	CopyString newtext = text.right(int(chkarg(2,0,strlen(gargstr(1)))));
	hoc_assign_str(hoc_pgargstr(1), newtext.string());
	return 1.;
}

static double l_is_name(void*) {
	return hoc_lookup(gargstr(1)) ? 1. : 0.;
}


extern "C" {
extern void hoc_free_symspace(Symbol*);
extern Object* hoc_newobj1(Symbol*, int);
extern Symlist* hoc_top_level_symlist;

extern Symbol* ivoc_alias_lookup(const char* name, Object* ob) {
	IvocAliases* a = (IvocAliases*)ob->aliases;
	if (a) {
		return a->lookup(name);
	}
	return NULL;
}

extern void ivoc_free_alias(Object* ob) {
	IvocAliases* a = (IvocAliases*)ob->aliases;
	if (a) delete a;
}

}

static double l_alias(void*) {
	char* name;
	Object* ob = *hoc_objgetarg(1);
	IvocAliases* a = (IvocAliases*)ob->aliases;
	Symbol* sym;

	if (!ifarg(2)) { // remove them all
		if (a) { delete a; }
		return 0.;
	}
	name = gargstr(2);
	if (!a) {
		a = new IvocAliases(ob);
	}
	sym = a->lookup(name);
	if (sym) {
		a->remove(sym);
	}
	if (ifarg(3)) {
		sym = a->install(name);
		if (hoc_is_object_arg(3)) {
			sym->u.object_ = *hoc_objgetarg(3);
			hoc_obj_ref(sym->u.object_);
			sym->type = OBJECTALIAS;
		}else{
			sym->u.pval = hoc_pgetarg(3);
			sym->type = VARALIAS;
		}
	}
	return 0.;
}

static Object** l_alias_list(void*) {
	// Assumes that a hoc String exists with constructor that takes a strdef
	Object* ob = *hoc_objgetarg(1);
	IvocAliases* a = (IvocAliases*)ob->aliases;
	OcList* list = new OcList();
	list->ref();
	Symbol* sl = hoc_lookup("List");
	Symbol* st = hoc_table_lookup("String", hoc_top_level_symlist);
	if (!st || st->type != TEMPLATE) {
printf("st=%p %s %d\n", st, st?st->name:"NULL", st?st->type:0);
		hoc_execerror("String is not a template", 0);
	}
	Object** po = hoc_temp_objvar(sl, list);
	(*po)->refcount++;
	int id = (*po)->index;
	if (a) {
		char buf[256];
		for (TableIterator(SymbolTable) i(*a->symtab_); i.more(); i.next()) {
			Symbol* sym = i.cur_value();
			hoc_pushstr(&sym->name);
			Object* sob = hoc_newobj1(st, 1);
			list->append(sob);
			--sob->refcount;
		}
	}
	(*po)->refcount--;
	return po;
}

// does o refer to ob
static int l_ref2(Object* o, Object* ob, int nr) {
	int i, total;
	if (!o) { return nr; }
	if (o->ctemplate->constructor) { return nr; }
	Symlist* sl = o->ctemplate->symtable;
	Symbol* s;
	if (sl) for (s = sl->first; s; s = s->next) {
		if (s->type == OBJECTVAR && s->cpublic < 2) {
			total = hoc_total_array_data(s, o->u.dataspace);
			for (i=0; i < total; ++i) {
				Object** obp = o->u.dataspace[s->u.oboff].pobj + i;
				if (*obp == ob) {
					if (total == 1) {
printf("   %s.%s\n", hoc_object_name(o), s->name);
					}else{
printf("   %s.%s[%d]\n", hoc_object_name(o), s->name, i);
					}
					++nr;
				}
			}
		}
	}
	return nr;
}

// does data refer to ob
static int l_ref1(Symlist* sl, Objectdata* data, Object* ob, int nr) {
	int i, total;
	Symbol* s;
	if (sl) for (s = sl->first; s; s = s->next) {
		if (s->type == OBJECTVAR && s->cpublic < 2) {
			total = hoc_total_array_data(s, data);
			for (i=0; i < total; ++i) {
				Object** obp = data[s->u.oboff].pobj + i;
				if (*obp == ob) {
					if (total == 1) {
printf("   %s\n",  s->name);
					}else{
printf("   %s[%d]\n", s->name, i);
					}
					++nr;
				}
			}
		}
	}
	return nr;
}
static int l_ref0(Symlist* sl, Object* ob, int nr) {
	Symbol* s;
	cTemplate* t;
	if (sl) for (s = sl->first; s; s = s->next) {
		if (s->type == TEMPLATE) {
			t = s->u.ctemplate;
			hoc_Item* q;
			ITERATE(q, t->olist) {
				nr = l_ref2(OBJ(q), ob, nr);
			}
		}
	}
	return nr;
}

static int l_ref3(Symbol* s, Object* ob, int nr) {
#if HAVE_IV
	hoc_Item* q;
	ITERATE(q, s->u.ctemplate->olist) {
		OcBox* b = (OcBox*)(OBJ(q)->u.this_pointer);
		if (b->keep_ref() == ob){
			printf("   %s.ref\n", hoc_object_name(OBJ(q)));
			++nr;
		}
	}
	return nr;
#else
	return 0;
#endif
}

static int l_ref4(Symbol* s, Object* ob, int nr) {
	hoc_Item* q;
	long i;
	ITERATE(q, s->u.ctemplate->olist) {
		OcList* list = (OcList*)(OBJ(q)->u.this_pointer);
		if (list->refs_items()) for (i = 0; i < list->count(); ++i) {
			if (list->object(i) == ob) {	
printf("   %s.object(%ld)\n", hoc_object_name(OBJ(q)), i);
				++nr;
			}
		}
	}
	return nr;
}

static double l_ref(void*) {
	Object* ob = *hoc_objgetarg(1);
	int nr = ob ? ob->refcount : 0;
	printf("%s has %d references\n", hoc_object_name(ob), nr);
	if (nr == 0) { return 0.; }
	nr = 0;
	nr = l_ref1(hoc_top_level_symlist, hoc_top_level_data, ob, nr);
	nr = l_ref0(hoc_top_level_symlist, ob, nr);
	// any due to boxes
	nr = l_ref3(hoc_table_lookup("HBox", hoc_built_in_symlist), ob, nr);
	nr = l_ref3(hoc_table_lookup("VBox", hoc_built_in_symlist), ob, nr);
	nr = l_ref4(hoc_table_lookup("List", hoc_built_in_symlist), ob, nr);
	
	printf("  found %d of them\n", nr);
	return (double)nr;
}

static double l_is_point(void*) {
	Object* ob = *hoc_objgetarg(1);
	return double(ob ? ob->ctemplate->is_point_ : 0);
}

static double l_is_artificial(void*) {
	Object* ob = *hoc_objgetarg(1);
	int type = ob ? ob->ctemplate->is_point_ : 0;
	if (type == 0) { return 0.; }
	return nrn_is_artificial(type) ? type : 0;
}

static Member_func l_members[] = {
	"substr", l_substr,
	"len", l_len,
	"head", l_head,
	"tail", l_tail,
	"right", l_right,
	"left", l_left,
	"is_name", l_is_name,
	"alias", l_alias,
	"references", l_ref,
	"is_point_process", l_is_point,
	"is_artificial", l_is_artificial,
	0,0
};

static Member_ret_obj_func l_obj_members[] = {
	"alias_list", l_alias_list,
	0,0
};

static void* l_cons(Object*) {
	return NULL;
}

static void l_destruct(void*) {
}

void StringFunctions_reg() {
	class2oc("StringFunctions", l_cons, l_destruct, l_members, NULL, l_obj_members, NULL);
}


IvocAliases::IvocAliases(Object* ob){
	ob_ = ob;
	ob_->aliases = (void*)this;
	symtab_ = new SymbolTable(20);
}

IvocAliases::~IvocAliases(){
	ob_->aliases = NULL;
	for (TableIterator(SymbolTable) i(*symtab_); i.more(); i.next()) {
		Symbol* sym = i.cur_value();
		hoc_free_symspace(sym);
		free(sym->name);
		free(sym);
	}
	delete symtab_;
}
Symbol* IvocAliases::lookup(const char* name){
	String s(name);
	Symbol* sym;
	if (!symtab_->find(sym, s)) {
		sym = NULL;
	}
	return sym;
}
Symbol* IvocAliases::install(const char* name){
	Symbol* sp;
	sp = (Symbol*)emalloc(sizeof(Symbol));
	sp->name = (char*)emalloc(strlen(name)+1);
	strcpy(sp->name, name);
	sp->type = VARALIAS;
	sp->cpublic = 0; // cannot be 2 or cannot be freed
	sp->extra = 0;
	sp->arayinfo = 0;
	String s(sp->name);
	symtab_->insert(s, sp);
	return sp;
}
void IvocAliases::remove(Symbol* sym){
	hoc_free_symspace(sym);
	String s(sym->name);
	symtab_->remove(s);
	free(sym->name);
	free(sym);
}

#include <../../nrnconf.h>
#include <stdio.h>
#include <InterViews/resource.h>
#include <OS/string.h>
#include <OS/table.h>
#include <OS/list.h>
#include "hoclist.h"
#if HAVE_IV
#include "graph.h"
#endif
#include "datapath.h"
#include "ivocvect.h"

#if !defined(CABLE)
// really belongs in vector.c but this is convenient since it will be
// present in ivoc but not in nrniv
void nrn_vecsim_add(void*, bool) {printf("nrn_vecsym_add implemented in nrniv\n");}
void nrn_vecsim_remove(void*) {printf("nrn_vecsym_remove implemented in nrniv\n");}
#if HAVE_IV
void graphLineRecDeleted(GraphLine*){}
void Graph::simgraph(){}
#endif
// another hack so ivoc will have these names which nrniv gets elsewhere
extern "C" {
int bbs_poll_;
void bbs_done(){}
void bbs_handle(){}
void nrnbbs_context_wait(){}
}

#ifdef WIN32
extern "C" {
void* dll_lookup(struct DLL*, const char*){return NULL;}
struct DLL* dll_load(const char*){return NULL;}
}
#endif
#endif

extern "C" {
#if CABLE
#include "nrnoc2iv.h"
#include "membfunc.h"
#else
#include "oc2iv.h"
#endif

#include "parse.h"
extern Symlist* hoc_built_in_symlist;
extern Symlist* hoc_top_level_symlist;
extern Objectdata* hoc_top_level_data;
}

/*static*/ class PathValue {
public:
	PathValue();
	~PathValue();
	CopyString* path;
	Symbol* sym;
	double original;
	char* str;
};
PathValue::PathValue() {
	path = NULL;
	str = NULL;
	sym = NULL;
}
PathValue::~PathValue() {
	if (path) {
		delete path;
	}
}

declareTable(PathTable, void*, PathValue*);
implementTable(PathTable, void*, PathValue*);
declarePtrList(StringList, char);
implementPtrList(StringList, char);


class HocDataPathImpl {
private:
	friend class HocDataPaths;
	HocDataPathImpl(int, int);
	~HocDataPathImpl();

	void search();
	void found(double*, const char*, Symbol*);
	void found(char**, const char*, Symbol*);
	PathValue* found_v(void*, const char*, Symbol*);

	void search(Objectdata*, Symlist*);
	void search_vectors();
#if CABLE
	void search(Section*);
	void search(Node*, double x);
	void search(Point_process*, Symbol*);
	void search(Prop*, double x);
#endif
private:
	PathTable* table_;	
	StringList strlist_;
	int size_, count_, found_so_far_;
	int pathstyle_;
};

#define sentinal 123456789.e15

static Symbol* sym_vec, *sym_v, *sym_vext, *sym_rallbranch, *sym_L, *sym_Ra;

HocDataPaths::HocDataPaths(int size, int pathstyle) {
	if (!sym_vec) {
		sym_vec = hoc_table_lookup("Vector", hoc_built_in_symlist);
#if CABLE
		sym_v = hoc_table_lookup("v", hoc_built_in_symlist);
		sym_vext = hoc_table_lookup("vext", hoc_built_in_symlist);
		sym_rallbranch = hoc_table_lookup("rallbranch", hoc_built_in_symlist);
		sym_L = hoc_table_lookup("L", hoc_built_in_symlist);
		sym_Ra = hoc_table_lookup("Ra", hoc_built_in_symlist);
#endif
	}
	impl_ = new HocDataPathImpl(size, pathstyle);
}

HocDataPaths::~HocDataPaths() {
	delete impl_;
}

int HocDataPaths::style() {
	return impl_->pathstyle_;
}

void HocDataPaths::append(double* pd) {
//	printf("HocDataPaths::append\n");
	PathValue* pv;
	if (pd && !impl_->table_->find(pv, (void*)pd)) {
		impl_->table_->insert((void*)pd, new PathValue);
		++impl_->count_;
	}
}

void HocDataPaths::search() {
//	printf("HocDataPaths::search\n");
	impl_->search();
	if (impl_->count_ > impl_->found_so_far_) {
//		printf("HocDataPaths::  didn't find paths to all the pointers\n");
// this has proved to be too often a false alarm since most panels are
// in boxes controlled by objects which have no reference but are
// deleted when the window is closed
	}
}

String* HocDataPaths::retrieve(double* pd) {
	assert(impl_->pathstyle_ != 2);
//	printf("HocDataPaths::retrieve\n");
	PathValue* pv;
	if (impl_->table_->find(pv, (void*)pd)) {
		return pv->path;
	}else{
		return NULL;
	}
}

Symbol* HocDataPaths::retrieve_sym(double* pd) {
//	printf("HocDataPaths::retrieve\n");
	PathValue* pv;
	if (impl_->table_->find(pv, (void*)pd)) {
		return pv->sym;
	}else{
		return NULL;
	}
}

void HocDataPaths::append(char** pd) {
//	printf("HocDataPaths::append\n");
	PathValue* pv;
	if (*pd && !impl_->table_->find(pv, (void*)pd)) {
		pv = new PathValue;
		pv->str = *pd;
		impl_->table_->insert((void*)pd, pv);
		++impl_->count_;
	}
}

String* HocDataPaths::retrieve(char** pd) {
//	printf("HocDataPaths::retrieve\n");
	PathValue* pv;
	if (impl_->table_->find(pv, (void*)pd)) {
		return pv->path;
	}else{
		return NULL;
	}
	
}

/*------------------------------*/
HocDataPathImpl::HocDataPathImpl(int size, int pathstyle) {
	pathstyle_ = pathstyle;
	table_ = new PathTable(size);
	size_ = size;
	count_ = 0;
	found_so_far_ = 0;
}

HocDataPathImpl::~HocDataPathImpl() {
	for (TableIterator(PathTable) i(*table_); i.more(); i.next()) {
		PathValue* pv = i.cur_value();
		delete pv;
	}
	delete table_;
}

void HocDataPathImpl::search() {
	found_so_far_ = 0;
   if (table_) {
	for (TableIterator(PathTable) i(*table_); i.more(); i.next()) {
		PathValue* pv = i.cur_value();
		if (pv->str) {
			char** pstr = (char**)i.cur_key();
			*pstr = NULL;
		}else{
			double* pd = (double*)i.cur_key();
			pv->original = *pd;
			*pd = sentinal;
		}
	}
   }
	if (pathstyle_ > 0) {
		search(hoc_top_level_data, hoc_built_in_symlist);
		search(hoc_top_level_data, hoc_top_level_symlist);
	}else{
		search(hoc_top_level_data, hoc_top_level_symlist);
		search(hoc_top_level_data, hoc_built_in_symlist);
	}
	if (found_so_far_ < count_) {
		search_vectors();
	}
   if (table_) {
	for (TableIterator(PathTable) i(*table_); i.more(); i.next()) {
		PathValue* pv = i.cur_value();
		if (pv->str) {
			char** pstr = (char**)i.cur_key();
			*pstr = pv->str;
		}else{
			double* pd = (double*)i.cur_key();
			*pd = pv->original;
		}
	}
   }
}

PathValue* HocDataPathImpl::found_v(void* v, const char* buf, Symbol* sym) {
	PathValue* pv;
    if (pathstyle_ != 2) {
	char path[500];
	CopyString cs("");
	long i, cnt;
	int len = 0;
	cnt = strlist_.count();
	for (i=0; i< cnt; ++i) {
		sprintf(path, "%s%s.", cs.string(), strlist_.item(i));
		cs = path;
	}
	sprintf(path, "%s%s", cs.string(), buf);
	if (!table_->find(pv, v)) {
		hoc_warning("table lookup failed for pointer for-", path);
		return NULL;
	}
	if (!pv->path) {
		pv->path = new CopyString(path);
		pv->sym = sym;
		++found_so_far_;
	}
//printf("HocDataPathImpl::found %s\n", path);
    }else{
	if (!table_->find(pv, v)) {
		hoc_warning("table lookup failed for pointer for-", sym->name);
		return NULL;
	}
	if (!pv->sym) {
		pv->sym = sym;
		++found_so_far_;
	}
    }
	return pv;
}

void HocDataPathImpl::found(double* pd, const char* buf, Symbol* sym) {
	PathValue* pv = found_v((void*)pd, buf, sym);
	if (pv) {
		*pd = pv->original;
	}
}

void HocDataPathImpl::found(char** pstr, const char* buf, Symbol* sym) {
	PathValue* pv = found_v((void*)pstr, buf,sym);
	if (pv) {
		*pstr = pv->str;
	}else{
		hoc_assign_str(pstr, "couldn't find");
	}
}

void HocDataPathImpl::search(Objectdata* od, Symlist* sl) {
	Symbol* sym;
	int i, total;
	char buf[200];
	CopyString cs("");
	if (sl)	for (sym = sl->first; sym; sym = sym->next) {
	  if (sym->cpublic != 2) {
		switch (sym->type) {
		case VAR: {
			double* pd;
			if (sym->subtype == NOTUSER) {
				pd = object_pval(sym, od);
				total = hoc_total_array_data(sym, od);
			}else if (sym->subtype == USERDOUBLE) {
				pd = sym->u.pval;
				total = 1;
			}else{
				break;
			}
			for (i=0; i < total; ++i) {
				if (pd[i] == sentinal) {
sprintf(buf, "%s%s", sym->name, hoc_araystr(sym, i, od));
					cs = buf;
					found(pd + i, cs.string(), sym);
				}
			}
			}
			break;
		case STRING: {
			char** pstr = object_pstr(sym, od);			
			if (*pstr == NULL) {
				sprintf(buf, "%s", sym->name);
				cs = buf;
				found(pstr, cs.string(), sym);
			}
			}
			break;
		case OBJECTVAR: {
			if (pathstyle_ > 0) { break; }
			Object** obp = object_pobj(sym, od);
			total = hoc_total_array_data(sym, od);
			for (i=0; i < total; ++i) if (obp[i] && ! obp[i]->recurse) {
				cTemplate* t = (obp[i])->ctemplate;
				if (!t->constructor) {
					// not the this pointer
					if (obp[i]->u.dataspace != od) {
sprintf(buf, "%s%s", sym->name, hoc_araystr(sym, i, od));
cs = buf;
strlist_.append((char*)cs.string());
obp[i]->recurse = 1;
search(obp[i]->u.dataspace, obp[i]->ctemplate->symtable);
obp[i]->recurse = 0;
strlist_.remove(strlist_.count()-1);
					}
				}else{
					/* point processes */
#if CABLE
					if (t->is_point_) {
sprintf(buf, "%s%s", sym->name, hoc_araystr(sym, i, od));
cs = buf;
strlist_.append((char*)cs.string());
search((Point_process*)obp[i]->u.this_pointer, sym);
strlist_.remove(strlist_.count()-1);
					}
#endif
					/* seclists, object lists */
				}
			}
			}
			break;
#if CABLE
		case SECTION: {
			total = hoc_total_array_data(sym, od);
			for (i=0; i < total; ++i) {
				hoc_Item** pitm = object_psecitm(sym, od);
				if (pitm[i]) {
sprintf(buf, "%s%s", sym->name, hoc_araystr(sym, i, od));
					cs = buf;
					strlist_.append((char*)cs.string());
					search(hocSEC(pitm[i]));
					strlist_.remove(strlist_.count()-1);
				}
			}
			}
			break;
#endif
		case TEMPLATE: {
			cTemplate* t = sym->u.ctemplate;
			hoc_Item* q;
			ITERATE (q, t->olist) {
				Object* obj = OBJ(q);
				sprintf(buf, "%s[%d]", sym->name, obj->index);
				cs = buf;
				strlist_.append((char*)cs.string());
				if (!t->constructor) {
					search(obj->u.dataspace, t->symtable);
				}else{
#if CABLE
					if (t->is_point_) {
search((Point_process*)obj->u.this_pointer, sym);
					}
#endif
				}
				strlist_.remove(strlist_.count()-1);
			}
			}
			break;
		}
	  }
	}
}

void HocDataPathImpl::search_vectors() {
	int i, cnt;
	char buf[200];
	CopyString cs("");
	cTemplate* t = sym_vec->u.ctemplate;
	hoc_Item* q;
	ITERATE (q, t->olist) {
		Object* obj = OBJ(q);
		sprintf(buf, "%s[%d]", sym_vec->name, obj->index);
		cs = buf;
		strlist_.append((char*)cs.string());
		Vect* vec = (Vect*)obj->u.this_pointer;
		int size = vec->capacity();
		double* pd = vector_vec(vec);
		for (i=0; i < size; ++i) {
			if (pd[i] == sentinal) {
				sprintf(buf, "x[%d]", i);
				found(pd + i, buf, sym_vec);
			}
		}
		strlist_.remove(strlist_.count()-1);
	}
}

#if CABLE

void HocDataPathImpl::search(Section* sec) {
	if (sec->prop->dparam[2].val == sentinal) {
		found(&sec->prop->dparam[2].val, "L", sym_L);
	}
	if (sec->prop->dparam[4].val == sentinal) {
		found(&sec->prop->dparam[4].val, "rallbranch", sym_rallbranch);
	}
	if (sec->prop->dparam[7].val == sentinal) {
		found(&sec->prop->dparam[7].val, "Ra", sym_Ra);
	}
	if (!sec->parentsec && sec->parentnode) {
		search(sec->parentnode, sec->prop->dparam[1].val);
	}
	for (int i = 0; i < sec->nnode; ++i) {
		search(sec->pnode[i], nrn_arc_position(sec, sec->pnode[i]));
	}
}
void HocDataPathImpl::search(Node* nd, double x) {
	char buf[100];
	int i, cnt;
	CopyString cs("");
	if (NODEV(nd) == sentinal) {
		sprintf(buf, "v(%g)", x);
		found(&NODEV(nd), buf, sym_v);
	}

#if EXTRACELLULAR
	if (nd->extnode) {
		int i;
		for (i=0; i < nlayer; ++i) {
			if (nd->extnode->v[i] == sentinal) {
				if (i == 0) {
					sprintf(buf, "vext(%g)", x);
				}else{
					sprintf(buf, "vext[%d](%g)", i, x);
				}
				found(&(nd->extnode->v[i]), buf, sym_vext);
			}
		}
	}
#endif

	Prop* p;
	for (p = nd->prop; p; p = p->next) {
		if (!memb_func[p->type].is_point) {
			search(p, x);
		}
	}	
}

void HocDataPathImpl::search(Point_process* pp, Symbol*) {
	if (pp->prop) {
		search(pp->prop, -1);
	}
}

void HocDataPathImpl::search(Prop* prop, double x) {
	char buf[200];
	int type = prop->type;
	Symbol* sym = memb_func[type].sym;
	Symbol* psym;
	double* pd;
	int i, imax, k=0, ir, kmax = sym->s_varn;

	for (k=0; k < kmax; ++k) {
		psym = sym->u.ppsym[k];
		if (psym->subtype == NRNPOINTER) { continue; }
		ir = psym->u.rng.index;
		if (memb_func[type].hoc_mech) {
			pd = prop->ob->u.dataspace[ir].pval;
		}else{
			pd = prop->param + ir;
		}
		imax = hoc_total_array_data(psym, 0);
		for (i=0; i < imax; ++i) {
			if (pd[i] == sentinal) {
if (x < 0) {
	sprintf(buf, "%s%s", psym->name, hoc_araystr(psym, i, 0));
}else{
	sprintf(buf, "%s%s(%g)", psym->name, hoc_araystr(psym, i, 0), x);
}
				found(pd + i, buf, psym);
			    }
		}
	}
}

#endif

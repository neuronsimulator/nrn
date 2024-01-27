#include <../../nrnconf.h>
#include <map>
#include <cstdio>
#include <InterViews/resource.h>
#include "hoclist.h"
#if HAVE_IV
#include "graph.h"
#endif
#include "datapath.h"
#include "ivocvect.h"

#include "nrnoc2iv.h"
#include "membfunc.h"

#include "parse.hpp"
extern Symlist* hoc_built_in_symlist;
extern Symlist* hoc_top_level_symlist;
extern Objectdata* hoc_top_level_data;

/*static*/ class PathValue {
  public:
    PathValue();
    ~PathValue() = default;
    std::string path{};
    Symbol* sym;
    double original;
    char* str;
};
PathValue::PathValue() {
    str = NULL;
    sym = NULL;
}

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
    void search_pysec();
    void search(Section*);
    void search(Node*, double x);
    void search(Point_process*, Symbol*);
    void search(Prop*, double x);

  private:
    std::map<void*, PathValue*> table_;
    std::vector<std::string> strlist_;
    int size_, count_, found_so_far_;
    int pathstyle_;
};

#define sentinal 123456789.e15

static Symbol *sym_vec, *sym_v, *sym_vext, *sym_rallbranch, *sym_L, *sym_Ra;

HocDataPaths::HocDataPaths(int size, int pathstyle) {
    if (!sym_vec) {
        sym_vec = hoc_table_lookup("Vector", hoc_built_in_symlist);
        sym_v = hoc_table_lookup("v", hoc_built_in_symlist);
        sym_vext = hoc_table_lookup("vext", hoc_built_in_symlist);
        sym_rallbranch = hoc_table_lookup("rallbranch", hoc_built_in_symlist);
        sym_L = hoc_table_lookup("L", hoc_built_in_symlist);
        sym_Ra = hoc_table_lookup("Ra", hoc_built_in_symlist);
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
    if (pd && impl_->table_.find((void*) pd) == impl_->table_.end()) {
        impl_->table_.emplace((void*) pd, new PathValue);
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

std::string HocDataPaths::retrieve(double* pd) const {
    assert(impl_->pathstyle_ != 2);
    //	printf("HocDataPaths::retrieve\n");
    auto const it = impl_->table_.find(pd);
    if (it != impl_->table_.end()) {
        return it->second->path;
    }
    return {};
}

Symbol* HocDataPaths::retrieve_sym(double* pd) const {
    //	printf("HocDataPaths::retrieve\n");
    auto const it = impl_->table_.find(pd);
    if (it != impl_->table_.end()) {
        return it->second->sym;
    }
    return nullptr;
}

void HocDataPaths::append(char** pd) {
    //	printf("HocDataPaths::append\n");
    if (*pd && impl_->table_.find((void*) pd) == impl_->table_.end()) {
        PathValue* pv = new PathValue;
        pv->str = *pd;
        impl_->table_.emplace((void*) pd, pv);
        ++impl_->count_;
    }
}

std::string HocDataPaths::retrieve(char** pd) const {
    //	printf("HocDataPaths::retrieve\n");
    auto const it = impl_->table_.find(pd);
    if (it != impl_->table_.end()) {
        return it->second->path;
    }
    return {};
}

/*------------------------------*/
HocDataPathImpl::HocDataPathImpl(int size, int pathstyle) {
    pathstyle_ = pathstyle;
    size_ = size;
    count_ = 0;
    found_so_far_ = 0;
}

HocDataPathImpl::~HocDataPathImpl() {
    for (auto& kv: table_) {
        PathValue* pv = kv.second;
        delete pv;
    }
}

void HocDataPathImpl::search() {
    found_so_far_ = 0;
    for (auto& it: table_) {
        PathValue* pv = it.second;
        if (pv->str) {
            char** pstr = (char**) it.first;
            *pstr = nullptr;
        } else {
            double* pd = (double*) it.first;
            pv->original = *pd;
            *pd = sentinal;
        }
    }
    if (pathstyle_ > 0) {
        search(hoc_top_level_data, hoc_built_in_symlist);
        search(hoc_top_level_data, hoc_top_level_symlist);
    } else {
        search(hoc_top_level_data, hoc_top_level_symlist);
        search(hoc_top_level_data, hoc_built_in_symlist);
    }
    if (found_so_far_ < count_) {
        search_pysec();
    }
    if (found_so_far_ < count_) {
        search_vectors();
    }
    for (auto& it: table_) {
        PathValue* pv = it.second;
        if (pv->str) {
            char** pstr = (char**) it.first;
            *pstr = pv->str;
        } else {
            double* pd = (double*) it.first;
            *pd = pv->original;
        }
    }
}

PathValue* HocDataPathImpl::found_v(void* v, const char* buf, Symbol* sym) {
    PathValue* pv;
    if (pathstyle_ != 2) {
        char path[500];
        std::string cs{};
        for (const auto& str: strlist_) {
            Sprintf(path, "%s%s.", cs.c_str(), str.c_str());
            cs = path;
        }
        Sprintf(path, "%s%s", cs.c_str(), buf);
        const auto& it = table_.find(v);
        if (it == table_.end()) {
            hoc_warning("table lookup failed for pointer for-", path);
            return nullptr;
        }
        pv = it->second;
        if (pv->path.empty()) {
            pv->path = path;
            pv->sym = sym;
            ++found_so_far_;
        }
        // printf("HocDataPathImpl::found %s\n", path);
    } else {
        const auto& it = table_.find(v);
        if (it == table_.end()) {
            hoc_warning("table lookup failed for pointer for-", sym->name);
            return nullptr;
        }
        pv = it->second;
        if (!pv->sym) {
            pv->sym = sym;
            ++found_so_far_;
        }
    }
    return pv;
}

void HocDataPathImpl::found(double* pd, const char* buf, Symbol* sym) {
    PathValue* pv = found_v((void*) pd, buf, sym);
    if (pv) {
        *pd = pv->original;
    }
}

void HocDataPathImpl::found(char** pstr, const char* buf, Symbol* sym) {
    PathValue* pv = found_v((void*) pstr, buf, sym);
    if (pv) {
        *pstr = pv->str;
    } else {
        hoc_assign_str(pstr, "couldn't find");
    }
}

void HocDataPathImpl::search(Objectdata* od, Symlist* sl) {
    Symbol* sym;
    int i, total;
    char buf[200];
    std::string cs{};
    if (sl)
        for (sym = sl->first; sym; sym = sym->next) {
            if (sym->cpublic != 2) {
                switch (sym->type) {
                case VAR: {
                    double* pd;
                    if (sym->subtype == NOTUSER) {
                        pd = object_pval(sym, od);
                        total = hoc_total_array_data(sym, od);
                    } else if (sym->subtype == USERDOUBLE) {
                        pd = sym->u.pval;
                        total = 1;
                    } else {
                        break;
                    }
                    for (i = 0; i < total; ++i) {
                        if (pd[i] == sentinal) {
                            Sprintf(buf, "%s%s", sym->name, hoc_araystr(sym, i, od));
                            cs = buf;
                            found(pd + i, cs.c_str(), sym);
                        }
                    }
                } break;
                case STRING: {
                    char** pstr = object_pstr(sym, od);
                    if (*pstr == NULL) {
                        Sprintf(buf, "%s", sym->name);
                        cs = buf;
                        found(pstr, cs.c_str(), sym);
                    }
                } break;
                case OBJECTVAR: {
                    if (pathstyle_ > 0) {
                        break;
                    }
                    Object** obp = object_pobj(sym, od);
                    total = hoc_total_array_data(sym, od);
                    for (i = 0; i < total; ++i)
                        if (obp[i] && !obp[i]->recurse) {
                            cTemplate* t = (obp[i])->ctemplate;
                            if (!t->constructor) {
                                // not the this pointer
                                if (obp[i]->u.dataspace != od) {
                                    Sprintf(buf, "%s%s", sym->name, hoc_araystr(sym, i, od));
                                    cs = buf;
                                    strlist_.push_back(cs);
                                    obp[i]->recurse = 1;
                                    search(obp[i]->u.dataspace, obp[i]->ctemplate->symtable);
                                    obp[i]->recurse = 0;
                                    strlist_.pop_back();
                                }
                            } else {
                                /* point processes */
                                if (t->is_point_) {
                                    Sprintf(buf, "%s%s", sym->name, hoc_araystr(sym, i, od));
                                    cs = buf;
                                    strlist_.push_back(cs);
                                    search((Point_process*) obp[i]->u.this_pointer, sym);
                                    strlist_.pop_back();
                                }
                                /* seclists, object lists */
                            }
                        }
                } break;
                case SECTION: {
                    total = hoc_total_array_data(sym, od);
                    for (i = 0; i < total; ++i) {
                        hoc_Item** pitm = object_psecitm(sym, od);
                        if (pitm[i]) {
                            Sprintf(buf, "%s%s", sym->name, hoc_araystr(sym, i, od));
                            cs = buf;
                            strlist_.push_back(cs);
                            search(hocSEC(pitm[i]));
                            strlist_.pop_back();
                        }
                    }
                } break;
                case TEMPLATE: {
                    cTemplate* t = sym->u.ctemplate;
                    hoc_Item* q;
                    ITERATE(q, t->olist) {
                        Object* obj = OBJ(q);
                        Sprintf(buf, "%s[%d]", sym->name, obj->index);
                        cs = buf;
                        strlist_.push_back(cs);
                        if (!t->constructor) {
                            search(obj->u.dataspace, t->symtable);
                        } else {
                            if (t->is_point_) {
                                search((Point_process*) obj->u.this_pointer, sym);
                            }
                        }
                        strlist_.pop_back();
                    }
                } break;
                }
            }
        }
}

void HocDataPathImpl::search_vectors() {
    char buf[200];
    std::string cs{};
    cTemplate* t = sym_vec->u.ctemplate;
    hoc_Item* q;
    ITERATE(q, t->olist) {
        Object* obj = OBJ(q);
        Sprintf(buf, "%s[%d]", sym_vec->name, obj->index);
        cs = buf;
        strlist_.push_back(cs);
        Vect* vec = (Vect*) obj->u.this_pointer;
        int size = vec->size();
        double* pd = vector_vec(vec);
        for (size_t i = 0; i < size; ++i) {
            if (pd[i] == sentinal) {
                Sprintf(buf, "x[%zu]", i);
                found(pd + i, buf, sym_vec);
            }
        }
        strlist_.pop_back();
    }
}

void HocDataPathImpl::search_pysec() {
#if USE_PYTHON
    std::string cs{};
    hoc_Item* qsec;
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        if (sec->prop && sec->prop->dparam[PROP_PY_INDEX].get<void*>()) {
            cs = secname(sec);
            strlist_.push_back(cs);
            search(sec);
            strlist_.pop_back();
        }
    }
#endif
}

void HocDataPathImpl::search(Section* sec) {
    if (sec->prop->dparam[2].get<double>() == sentinal) {
        found(&(sec->prop->dparam[2].literal_value<double>()), "L", sym_L);
    }
    if (sec->prop->dparam[4].get<double>() == sentinal) {
        found(&(sec->prop->dparam[4].literal_value<double>()), "rallbranch", sym_rallbranch);
    }
    if (sec->prop->dparam[7].get<double>() == sentinal) {
        found(&(sec->prop->dparam[7].literal_value<double>()), "Ra", sym_Ra);
    }
    if (!sec->parentsec && sec->parentnode) {
        search(sec->parentnode, sec->prop->dparam[1].get<double>());
    }
    for (int i = 0; i < sec->nnode; ++i) {
        search(sec->pnode[i], nrn_arc_position(sec, sec->pnode[i]));
    }
}
void HocDataPathImpl::search(Node* nd, double x) {
    char buf[100];
    if (NODEV(nd) == sentinal) {
        Sprintf(buf, "v(%g)", x);
        // the conversion below yields a pointer that is potentially invalidated
        // by almost any Node operation
        found(static_cast<double*>(nd->v_handle()), buf, sym_v);
    }

#if EXTRACELLULAR
    if (nd->extnode) {
        int i;
        for (i = 0; i < nlayer; ++i) {
            if (nd->extnode->v[i] == sentinal) {
                if (i == 0) {
                    Sprintf(buf, "vext(%g)", x);
                } else {
                    Sprintf(buf, "vext[%d](%g)", i, x);
                }
                found(&(nd->extnode->v[i]), buf, sym_vext);
            }
        }
    }
#endif

    Prop* p;
    for (p = nd->prop; p; p = p->next) {
        if (!memb_func[p->_type].is_point) {
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
    int type = prop->_type;
    Symbol* sym = memb_func[type].sym;
    Symbol* psym;
    double* pd;
    int i, imax, k = 0, ir, kmax = sym->s_varn;

    for (k = 0; k < kmax; ++k) {
        psym = sym->u.ppsym[k];
        if (psym->subtype == NRNPOINTER) {
            continue;
        }
        ir = psym->u.rng.index;
        if (memb_func[type].hoc_mech) {
            pd = prop->ob->u.dataspace[ir].pval;
        } else {
            if (type == EXTRACELL && ir == neuron::extracellular::vext_pseudoindex()) {
                // skip as it was handled by caller
                continue;
            } else {
                pd = static_cast<double*>(prop->param_handle_legacy(ir));
            }
        }
        imax = hoc_total_array_data(psym, 0);
        for (i = 0; i < imax; ++i) {
            if (pd[i] == sentinal) {
                if (x < 0) {
                    Sprintf(buf, "%s%s", psym->name, hoc_araystr(psym, i, 0));
                } else {
                    Sprintf(buf, "%s%s(%g)", psym->name, hoc_araystr(psym, i, 0), x);
                }
                found(pd + i, buf, psym);
            }
        }
    }
}

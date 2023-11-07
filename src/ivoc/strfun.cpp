#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>
#include "classreg.h"
#include "oc2iv.h"
#include <string.h>
#include <regex>
// for alias
#include <symdir.h>
#include <oclist.h>
#include <parse.hpp>
// for references
#include <hoclist.h>
#if HAVE_IV
#include <ocbox.h>
#endif
extern Objectdata* hoc_top_level_data;
extern Symlist* hoc_built_in_symlist;
extern int nrn_is_artificial(int);

extern int hoc_return_type_code;

static double l_substr(void*) {
    char* s1 = gargstr(1);
    char* s2 = gargstr(2);
    char* p = strstr(s1, s2);
    hoc_return_type_code = 1;  // integer
    if (p) {
        return double(p - s1);
    } else {
        return -1.;
    }
}

static double l_len(void*) {
    hoc_return_type_code = 1;  // integer
    return double(strlen(gargstr(1)));
}

static double l_head(void*) {
    std::string text(gargstr(1));
    {  // Clean the text so we keep only the first line
       // Imitation of std::multiline in our case
        std::regex r("^(.*)(\n|$)");
        std::smatch sm;
        std::regex_search(text, sm, r);
        text = sm[1];
    }
    int i = -1;
    std::string result{};
    try {
        std::regex r(gargstr(2), std::regex::egrep);
        if (std::smatch sm; std::regex_search(text, sm, r)) {
            i = sm.position();
            result = sm.prefix().str();
        }
    } catch (const std::regex_error& e) {
        std::cerr << e.what() << std::endl;
    }
    char** head = hoc_pgargstr(3);
    hoc_assign_str(head, result.c_str());
    hoc_return_type_code = 1;  // integer
    return double(i);
}

static double l_tail(void*) {
    std::string text(gargstr(1));
    {  // Clean the text so we keep only the first line
       // Imitation of std::multiline in our case
        std::regex r("^(.*)(\n|$)");
        std::smatch sm;
        std::regex_search(text, sm, r);
        text = sm[1];
    }
    int i = -1;
    std::string result{};
    try {
        std::regex r(gargstr(2), std::regex::egrep);
        if (std::smatch sm; std::regex_search(text, sm, r)) {
            i = sm.position() + sm.length();
            result = sm.suffix().str();
        }
    } catch (const std::regex_error& e) {
        std::cerr << e.what() << std::endl;
    }
    char** tail = hoc_pgargstr(3);
    hoc_assign_str(tail, result.c_str());
    hoc_return_type_code = 1;  // integer
    return double(i);
}

static double l_ltrim(void*) {
    std::string s(gargstr(1));
    std::string chars = " \r\n\t\f\v";
    if (ifarg(3)) {
        chars = gargstr(3);
    }
    s.erase(0, s.find_first_not_of(chars));

    char** ret = hoc_pgargstr(2);
    hoc_assign_str(ret, s.c_str());
    return 0.;
}

static double l_rtrim(void*) {
    std::string s(gargstr(1));
    std::string chars = " \r\n\t\f\v";
    if (ifarg(3)) {
        chars = gargstr(3);
    }
    s.erase(s.find_last_not_of(chars) + 1);

    char** ret = hoc_pgargstr(2);
    hoc_assign_str(ret, s.c_str());
    return 0.;
}

static double l_left(void*) {
    std::string text(gargstr(1));
    std::string newtext = text.substr(0, int(chkarg(2, 0, strlen(gargstr(1)))));
    hoc_assign_str(hoc_pgargstr(1), newtext.c_str());
    return 1.;
}

static double l_right(void*) {
    std::string text(gargstr(1));
    std::string newtext = text.substr(int(chkarg(2, 0, strlen(gargstr(1)))));
    hoc_assign_str(hoc_pgargstr(1), newtext.c_str());
    return 1.;
}

static double l_is_name(void*) {
    hoc_return_type_code = 2;  // boolean
    return hoc_lookup(gargstr(1)) ? 1. : 0.;
}


extern void hoc_free_symspace(Symbol*);
extern Object* hoc_newobj1(Symbol*, int);
extern Symlist* hoc_top_level_symlist;

extern Symbol* ivoc_alias_lookup(const char* name, Object* ob) {
    Symbol* s{};
    IvocAliases* a = (IvocAliases*) ob->aliases;
    if (a) {
        s = a->lookup(name);
    }
    return s;
}

extern void ivoc_free_alias(Object* ob) {
    IvocAliases* a = (IvocAliases*) ob->aliases;
    if (a)
        delete a;
}


static double l_alias(void*) {
    char* name;
    Object* ob = *hoc_objgetarg(1);
    IvocAliases* a = (IvocAliases*) ob->aliases;
    Symbol* sym;

    if (!ifarg(2)) {  // remove them all
        if (a) {
            delete a;
        }
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
        } else {
            sym->u.pval = hoc_pgetarg(3);
            sym->type = VARALIAS;
        }
    }
    return 0.;
}

static Object** l_alias_list(void*) {
    // Assumes that a hoc String exists with constructor that takes a strdef
    Object* ob = *hoc_objgetarg(1);
    IvocAliases* a = (IvocAliases*) ob->aliases;
    OcList* list = new OcList();
    list->ref();
    Symbol* sl = hoc_lookup("List");
    Symbol* st = hoc_table_lookup("String", hoc_top_level_symlist);
    if (!st || st->type != TEMPLATE) {
        hoc_execerror("String is not a HOC template", 0);
    }
    Object** po = hoc_temp_objvar(sl, list);
    (*po)->refcount++;
    int id = (*po)->index;
    if (a) {
        for (auto& kv: a->symtab_) {
            Symbol* sym = kv.second;
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
    if (!o) {
        return nr;
    }
    if (o->ctemplate->constructor) {
        return nr;
    }
    Symlist* sl = o->ctemplate->symtable;
    Symbol* s;
    if (sl)
        for (s = sl->first; s; s = s->next) {
            if (s->type == OBJECTVAR && s->cpublic < 2) {
                total = hoc_total_array_data(s, o->u.dataspace);
                for (i = 0; i < total; ++i) {
                    Object** obp = o->u.dataspace[s->u.oboff].pobj + i;
                    if (*obp == ob) {
                        if (total == 1) {
                            Printf("   %s.%s\n", hoc_object_name(o), s->name);
                        } else {
                            Printf("   %s.%s[%d]\n", hoc_object_name(o), s->name, i);
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
    if (sl)
        for (s = sl->first; s; s = s->next) {
            if (s->type == OBJECTVAR && s->cpublic < 2) {
                total = hoc_total_array_data(s, data);
                for (i = 0; i < total; ++i) {
                    Object** obp = data[s->u.oboff].pobj + i;
                    if (*obp == ob) {
                        if (total == 1) {
                            Printf("   %s\n", s->name);
                        } else {
                            Printf("   %s[%d]\n", s->name, i);
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
    if (sl)
        for (s = sl->first; s; s = s->next) {
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
        OcBox* b = (OcBox*) (OBJ(q)->u.this_pointer);
        if (b->keep_ref() == ob) {
            Printf("   %s.ref\n", hoc_object_name(OBJ(q)));
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
        OcList* list = (OcList*) (OBJ(q)->u.this_pointer);
        if (list->refs_items())
            for (i = 0; i < list->count(); ++i) {
                if (list->object(i) == ob) {
                    Printf("   %s.object(%ld)\n", hoc_object_name(OBJ(q)), i);
                    ++nr;
                }
            }
    }
    return nr;
}

static double l_ref(void*) {
    Object* ob = *hoc_objgetarg(1);
    int nr = ob ? ob->refcount : 0;
    Printf("%s has %d references\n", hoc_object_name(ob), nr);
    hoc_return_type_code = 1;  // integer
    if (nr == 0) {
        return 0.;
    }
    nr = 0;
    nr = l_ref1(hoc_top_level_symlist, hoc_top_level_data, ob, nr);
    nr = l_ref0(hoc_top_level_symlist, ob, nr);
    // any due to boxes
    nr = l_ref3(hoc_table_lookup("HBox", hoc_built_in_symlist), ob, nr);
    nr = l_ref3(hoc_table_lookup("VBox", hoc_built_in_symlist), ob, nr);
    nr = l_ref4(hoc_table_lookup("List", hoc_built_in_symlist), ob, nr);

    Printf("  found %d of them\n", nr);
    return (double) nr;
}

static double l_is_point(void*) {
    Object* ob = *hoc_objgetarg(1);
    hoc_return_type_code = 1;  // integer
    return double(ob ? ob->ctemplate->is_point_ : 0);
}

static double l_is_artificial(void*) {
    Object* ob = *hoc_objgetarg(1);
    int type = ob ? ob->ctemplate->is_point_ : 0;
    hoc_return_type_code = 1;  // integer
    if (type == 0) {
        return 0.;
    }
    return nrn_is_artificial(type) ? type : 0;
}

static Member_func l_members[] = {{"substr", l_substr},
                                  {"len", l_len},
                                  {"head", l_head},
                                  {"tail", l_tail},
                                  {"ltrim", l_ltrim},
                                  {"rtrim", l_rtrim},
                                  {"right", l_right},
                                  {"left", l_left},
                                  {"is_name", l_is_name},
                                  {"alias", l_alias},
                                  {"references", l_ref},
                                  {"is_point_process", l_is_point},
                                  {"is_artificial", l_is_artificial},
                                  {0, 0}};

static Member_ret_obj_func l_obj_members[] = {{"alias_list", l_alias_list}, {0, 0}};

static void* l_cons(Object*) {
    return nullptr;
}

void StringFunctions_reg() {
    class2oc("StringFunctions", l_cons, nullptr, l_members, nullptr, l_obj_members, nullptr);
}


IvocAliases::IvocAliases(Object* ob) {
    ob_ = ob;
    ob_->aliases = (void*) this;
}

IvocAliases::~IvocAliases() {
    ob_->aliases = nullptr;
    for (auto& kv: symtab_) {
        Symbol* sym = kv.second;
        hoc_free_symspace(sym);
        free(sym->name);
        free(sym);
    }
}
Symbol* IvocAliases::lookup(const char* name) {
    const auto& it = symtab_.find(name);
    if (it != symtab_.end()) {
        return it->second;
    }
    return nullptr;
}
Symbol* IvocAliases::install(const char* name) {
    Symbol* sp;
    sp = (Symbol*) emalloc(sizeof(Symbol));
    sp->name = static_cast<char*>(emalloc(strlen(name) + 1));
    strcpy(sp->name, name);
    sp->type = VARALIAS;
    sp->cpublic = 0;  // cannot be 2 or cannot be freed
    sp->extra = nullptr;
    sp->arayinfo = nullptr;
    symtab_.try_emplace(sp->name, sp);
    return sp;
}
void IvocAliases::remove(Symbol* sym) {
    hoc_free_symspace(sym);
    auto it = symtab_.find(sym->name);
    symtab_.erase(it);
    free(sym->name);
    free(sym);
}

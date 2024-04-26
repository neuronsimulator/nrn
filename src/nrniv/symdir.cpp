#include <../../nrnconf.h>

#include <fmt/format.h>
#include <tuple>

#include <stdlib.h>
#include <InterViews/resource.h>
#include <stdio.h>
#include "ocobserv.h"
#include "utils/enumerate.h"

#include "nrniv_mf.h"
#include "nrnoc2iv.h"

#include "membfunc.h"
#include "parse.hpp"
#include "hoclist.h"
extern Symlist* hoc_symlist;
extern Objectdata* hoc_top_level_data;
extern Symlist *hoc_built_in_symlist, *hoc_top_level_symlist;
#include "string.h"
#include "symdir.h"

#include "nrnsymdiritem.h"

class SymDirectoryImpl: public Observer {
  public:
    void disconnect(Observable*);  // watching an object
    void update(Observable*);      // watching a template
  private:
    friend class SymDirectory;
    Section* sec_ = nullptr;
    Object* obj_ = nullptr;
    cTemplate* t_ = nullptr;

    std::vector<SymbolItem*> symbol_lists_{};
    std::string path_{};

    void load(int type);
    void load(int type, Symlist*);
    //	void load(Symbol*);
    void load_section();
    void load_object();
    void load_aliases();
    void load_template();
    void load_mechanism(const Prop*, int, const std::string&);
    void append(Symbol* sym, Objectdata* od, Object* o = NULL);
    void append(Object*);
    void un_append(Object*);
    void make_pathname(const std::string&, const std::string&, const std::string&, char s = '.');
    void sort();
};

static bool compare_entries(const SymbolItem* e1, const SymbolItem* e2) {
    // Cannot use std::tie because array_index return an r-value ref
    return std::make_tuple(e1->name(), e1->array_index()) < std::make_tuple(e2->name(), e2->array_index());
};

void SymDirectoryImpl::sort() {
    std::sort(symbol_lists_.begin(), symbol_lists_.end(), compare_entries);
}

// SymDirectory
SymDirectory::SymDirectory(const std::string& parent_path,
                           Object* parent_obj,
                           Symbol* sym,
                           int array_index,
                           int) {
    impl_ = new SymDirectoryImpl();
    Objectdata* obd = parent_obj ? parent_obj->u.dataspace : hoc_top_level_data;
    char suffix = sym->type == TEMPLATE ? '_' : '.';
    impl_->make_pathname(parent_path,
                         sym->name,
                         hoc_araystr(sym, array_index, obd),
                         suffix);
    switch (sym->type) {
    case SECTION:
        if (object_psecitm(sym, obd)[array_index]) {
            impl_->sec_ = hocSEC(object_psecitm(sym, obd)[array_index]);
            section_ref(impl_->sec_);
            impl_->load_section();
        }
        break;
    case OBJECTVAR:
        impl_->obj_ = object_pobj(sym, obd)[array_index];
        if (impl_->obj_) {
            ObjObservable::Attach(impl_->obj_, impl_);
            impl_->load_object();
        }
        break;
    case TEMPLATE:
        impl_->t_ = sym->u.ctemplate;
        ClassObservable::Attach(impl_->t_, impl_);
        impl_->load_template();
        break;
    case OBJECTALIAS:
        impl_->obj_ = sym->u.object_;
        if (impl_->obj_) {
            ObjObservable::Attach(impl_->obj_, impl_);
            impl_->load_object();
        }
        break;
    default:
        hoc_execerror("Don't know how to make a directory out of", path().c_str());
        break;
    }
    impl_->sort();
}
SymDirectory::SymDirectory(Object* ob)
    : impl_(new SymDirectoryImpl())
{
    impl_->obj_ = ob;
    impl_->make_pathname("", hoc_object_name(ob), "", '.');
    ObjObservable::Attach(impl_->obj_, impl_);
    impl_->load_object();
    impl_->sort();
}

bool SymDirectory::is_pysec(int index) const {
    SymbolItem* si = impl_->symbol_lists_.at(index);
    return si->pysec_ != nullptr;
}
SymDirectory* SymDirectory::newsymdir(int index) {
    SymbolItem* si = impl_->symbol_lists_.at(index);
    SymDirectory* d = new SymDirectory();
    if (si->pysec_type_ == PYSECOBJ) {
        nrn_symdir_load_pysec(d->impl_->symbol_lists_, si->pysec_);
    } else {
        d->impl_->sec_ = (Section*) si->pysec_;
        section_ref(d->impl_->sec_);
        d->impl_->load_section();
    }
    d->impl_->path_ = path() + si->name() + ".";
    d->impl_->sort();
    return d;
}

SymDirectory::SymDirectory()
    : impl_(new SymDirectoryImpl)
{}

SymDirectory::SymDirectory(int type)
    : impl_(new SymDirectoryImpl())
{
    ParseTopLevel ptl;
    ptl.save();
    impl_->load(type);
    impl_->sort();
    ptl.restore();
}

SymDirectory::~SymDirectory() {
    for (auto& item: impl_->symbol_lists_) {
        delete item;
    }
    impl_->symbol_lists_.clear();
    impl_->symbol_lists_.shrink_to_fit();
    if (impl_->obj_) {
        ObjObservable::Detach(impl_->obj_, impl_);
    }
    if (impl_->t_) {
        ClassObservable::Detach(impl_->t_, impl_);
    }
    if (impl_->sec_) {
        section_unref(impl_->sec_);
    }
    delete impl_;
}
void SymDirectoryImpl::disconnect(Observable*) {
    for (auto& item: symbol_lists_) {
        delete item;
    }
    symbol_lists_.clear();
    symbol_lists_.shrink_to_fit();
    obj_ = NULL;
}

void SymDirectoryImpl::update(Observable* obs) {
    if (t_) {  // watching a template
        ClassObservable* co = (ClassObservable*) obs;
        Object* ob = co->object();
        switch (co->message()) {
        case ClassObservable::Delete:
            un_append(ob);
            break;
        case ClassObservable::Create:
            append(ob);
            break;
        }
    }
}
double* SymDirectory::variable(int index) {
    Object* ob = object();
    Symbol* sym = symbol(index);
    // printf("::variable index=%d sym=%s ob=%s\n", index,
    // sym?sym->name:"SYM0",hoc_object_name(ob));
    if (sym)
        switch (sym->type) {
        case VAR:
            if (ob && ob->ctemplate->constructor) {
                extern double* ivoc_vector_ptr(Object*, int);
                if (is_obj_type(ob, "Vector")) {
                    return ivoc_vector_ptr(ob, index);
                } else {
                    return NULL;
                }
            } else {
                Objectdata* od;
                if (ob) {
                    od = ob->u.dataspace;
                } else if (sym->subtype == USERDOUBLE) {
                    return sym->u.pval + array_index(index);
                } else {
                    od = hoc_objectdata;
                }
                return od[sym->u.oboff].pval + array_index(index);
            }
        case RANGEVAR:
            if (ob && ob->ctemplate->is_point_) {
                return static_cast<double*>(point_process_pointer(
                    (Point_process*) ob->u.this_pointer, sym, array_index(index)));
            }
            break;
        }
    else {
        std::string buf = path() + name(index);
        if (whole_vector(index)) {  // rangevar case for [all]
            // replace [all] with [0]
            auto pos = buf.find("[all]");
            buf.replace(pos, 5, "[0]");
        }
        return hoc_val_pointer(buf.c_str());
    }
    return nullptr;
}

int SymDirectory::whole_vector(int index) {
    return impl_->symbol_lists_.at(index)->whole_vector();
}

const std::string& SymDirectory::path() const {
    return impl_->path_;
}
int SymDirectory::count() const {
    return impl_->symbol_lists_.size();
}
const std::string& SymDirectory::name(int index) const {
    return impl_->symbol_lists_.at(index)->name();
}
int SymDirectory::array_index(int i) const {
    return impl_->symbol_lists_.at(i)->array_index();
}

int SymDirectory::index(const std::string& name) const {
    for (const auto&& [i, symbol]: enumerate(impl_->symbol_lists_)) {
        if (name == symbol->name()) {
            return i;
        }
    }
    return -1;
}
void SymDirectory::whole_name(int index, std::string& s) const {
    auto s1 = impl_->path_;
    auto s2 = name(index);
    s = s1 + s2;
}
bool SymDirectory::is_directory(int index) const {
    return impl_->symbol_lists_.at(index)->is_directory();
}
bool SymDirectory::match(const std::string&, const std::string&) {
    return true;
}
Symbol* SymDirectory::symbol(int index) const {
    return impl_->symbol_lists_.at(index)->symbol();
}
Object* SymDirectory::object() const {
    return impl_->obj_;
}

Object* SymDirectory::obj(int index) {
    return impl_->symbol_lists_.at(index)->object();
}

// SymbolItem
SymbolItem::SymbolItem(std::string&& n, int whole_array)
    : name_(n)
    , whole_array_(whole_array) {}

SymbolItem::SymbolItem(Symbol* sym, Objectdata* od, int index, int whole_array)
    : symbol_(sym)
    , index_(index)
    , whole_array_(whole_array)
{
    if (ISARRAY(sym)) {
        if (whole_array_) {
            name_ = fmt::format("{}{}", sym->name, "[all]");
        } else {
            if (od) {
                name_ = fmt::format("{}{}", sym->name, hoc_araystr(sym, index, od));
            } else {
                name_ = fmt::format("{}[{}]", sym->name, index);
            }
        }
    } else {
        name_ = sym->name;
    }
}

int SymbolItem::whole_vector() {
    return whole_array_;
}

SymbolItem::SymbolItem(Object* ob)
    : name_(std::to_string(ob->index))
    , ob_(ob)
{}

void SymbolItem::no_object() {
    ob_ = nullptr;
    name_ = "Deleted";
}

SymbolItem::~SymbolItem() {}

bool SymbolItem::is_directory() const {
    if (symbol_)
        switch (symbol_->type) {
        case SECTION:
        case OBJECTVAR:
        case TEMPLATE:
        case OBJECTALIAS:
            //	case SECTIONLIST:
            //	case MECHANISM:
            return true;
        }
    if (ob_) {
        return true;
    }
    if (pysec_) {
        return true;
    }
    return false;
}

void SymDirectoryImpl::make_pathname(const std::string& parent,
                                     const std::string& name,
                                     const std::string& index,
                                     char suffix) {
    path_ = parent + name + index + std::to_string(suffix);
}


void SymDirectoryImpl::load(int type) {
    switch (type) {
    case TEMPLATE:
        load(type, hoc_built_in_symlist);
        load(type, hoc_top_level_symlist);
        break;
    case RANGEVAR:
        load(type, hoc_built_in_symlist);
        break;
    case PYSEC:
        path_ = "_pysec.";
        nrn_symdir_load_pysec(symbol_lists_, NULL);
        break;
    default:
        load(type, hoc_symlist);
        if (hoc_symlist != hoc_built_in_symlist) {
            Objectdata* sav = hoc_objectdata;
            hoc_objectdata = NULL;
            load(type, hoc_built_in_symlist);
            hoc_objectdata = sav;
        }
        if (hoc_symlist != hoc_top_level_symlist) {
            load(type, hoc_top_level_symlist);
        }
    }
}

void SymDirectoryImpl::load(int type, Symlist* sl) {
    for (Symbol* sym = sl->first; sym; sym = sym->next) {
        if (type == -1) {
            switch (sym->type) {
            case SECTION:
            case OBJECTVAR:
            case VAR:
            case TEMPLATE:
                append(sym, hoc_objectdata);
            }
        } else if (sym->type == type) {
            append(sym, hoc_objectdata);
        }
    }
}

void SymDirectoryImpl::load_object() {
    Symlist* sl = obj_->ctemplate->symtable;
    Objectdata* od;
    if (obj_->ctemplate->constructor) {
        od = NULL;
    } else {
        od = obj_->u.dataspace;
    }
    if (obj_->aliases) {
        load_aliases();
    }
    if (sl)
        for (Symbol* s = sl->first; s; s = s->next) {
            if (s->cpublic) {
                append(s, od, obj_);
            }
        }
}

void SymDirectoryImpl::load_aliases() {
    IvocAliases* a = (IvocAliases*) obj_->aliases;
    if (!a)
        return;
    for (const auto& [_, s]: a->symtab_) {
        append(s, nullptr, obj_);
    }
}

void SymDirectoryImpl::load_template() {
    hoc_Item* q;
    ITERATE(q, t_->olist) {
        append(OBJ(q));
    }
}

void SymDirectoryImpl::load_section() {
    Section* sec = sec_;
    int n = sec->nnode;

    int i = 0;
    double x = nrn_arc_position(sec, sec->pnode[0]);
    auto xarg = fmt::format("( {:g} )", x);
    symbol_lists_.push_back(new SymbolItem(fmt::format("v{}", xarg)));
    nrn_pushsec(sec);
    Node* nd = sec->pnode[i];
    for (const Prop* p = nd->prop; p; p = p->next) {
        load_mechanism(p, 0, xarg);
    }
    nrn_popsec();
}

void SymDirectoryImpl::load_mechanism(const Prop* p, int vartype, const std::string& xarg) {
    int type = p->_type;
    if (memb_func[type].is_point) {
        return;
    }
    char buf[200];
    Symbol* msym = memb_func[type].sym;
    int cnt = msym->s_varn;
    for (int i = 0; i < cnt; ++i) {
        const Symbol* sym = msym->u.ppsym[i];
        if (nrn_vartype(sym) == vartype || vartype == 0) {
            if (ISARRAY(sym)) {
                int n = hoc_total_array_data(sym, 0);
                if (n > 5) {
                    auto buf = fmt::format("{}[all]{}", sym->name, xarg);
                    symbol_lists_.push_back(new SymbolItem(std::move(buf), n));
                }
                {
                    auto buf = fmt::format("{}[{}]{}", sym->name, 0, xarg);
                    symbol_lists_.push_back(new SymbolItem(std::move(buf)));
                }
                {
                    auto buf = fmt::format("{}[{}]{}", sym->name, n - 1, xarg);
                    symbol_lists_.push_back(new SymbolItem(std::move(buf)));
                }
            } else {
                auto buf = sym->name + xarg;
                symbol_lists_.push_back(new SymbolItem(std::move(buf)));
            }
        }
    }
}

void SymDirectoryImpl::append(Symbol* sym, Objectdata* od, Object* o) {
    if (ISARRAY(sym)) {
        int i, n = 1;
        if (od) {
            n = hoc_total_array_data(sym, od);
        } else {  // Vector
            if (is_obj_type(o, "Vector")) {
                extern int ivoc_vector_size(Object*);
                n = ivoc_vector_size(o);
            }
        }
        if (n > 5 && sym->type == VAR) {
            symbol_lists_.push_back(new SymbolItem(sym, od, 0, n));
        }
        for (i = 0; i < n; ++i) {
            symbol_lists_.push_back(new SymbolItem(sym, od, i));
            if (i > 5) {
                break;
            }
        }
        if (i < n - 1) {
            symbol_lists_.push_back(new SymbolItem(sym, od, n - 1));
        }
    } else {
        symbol_lists_.push_back(new SymbolItem(sym, od, 0));
    }
}

void SymDirectoryImpl::append(Object* ob) {
    symbol_lists_.push_back(new SymbolItem(ob));
}
void SymDirectoryImpl::un_append(Object* ob) {
    for (auto& symbol: symbol_lists_) {
        if (symbol->object() == ob) {
            symbol->no_object();
            break;
        }
    }
}

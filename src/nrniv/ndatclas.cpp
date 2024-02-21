#include <../../nrnconf.h>
#include <stdio.h>
#include <InterViews/resource.h>
#include "nrnoc2iv.h"
#include "classreg.h"

#include "membfunc.h"
#include "parse.hpp"
extern Prop* prop_alloc(Prop**, int, Node*);
extern void single_prop_free(Prop*);
extern Symlist* hoc_built_in_symlist;
extern Symlist* hoc_top_level_symlist;

//----------------------------------------------------
/* static */ class NrnPropertyImpl {
    friend class NrnProperty;
    NrnPropertyImpl(Prop*);
    ~NrnPropertyImpl();

    Prop* p_;
    int iterator_;
    Symbol* sym_;
    bool del_;
};
NrnPropertyImpl::NrnPropertyImpl(Prop* p) {
    p_ = p;
    iterator_ = -1;
    sym_ = memb_func[p_->_type].sym;
    del_ = false;
}

NrnPropertyImpl::~NrnPropertyImpl() {
    if (del_ && p_) {
        // printf("NrnProperty freed prop\n");
        single_prop_free(p_);
    }
}

//---------------------------------------------------
/* static */ class SectionListImpl {
    friend class SectionList;
    Object* ob_;
    struct hoc_Item* itr_;
    struct hoc_Item* list_;
};

//----------------------------------------------------
#if 0
/* static */ class NrnSectionImpl {
	friend NrnSection
	NrnSectionImpl(Section*);
	~NrnSectionImpl();
	
	Section* sec_;
};
NrnSectionImpl::NrnSectionImpl(Section* sec) {
	sec_ = sec;
	section_ref(sec);
}
NrnSectionImpl::~NrnSectionImpl() {
	section_unref(sec_);
}
#endif
//----------------------------------------------------
NrnProperty::NrnProperty(Prop* p) {
    npi_ = new NrnPropertyImpl(p);
}

NrnProperty::NrnProperty(const char* name) {
    Symbol* sym = hoc_table_lookup(name, hoc_built_in_symlist);
    if (!sym) {
        sym = hoc_table_lookup(name, hoc_top_level_symlist);
    }
    if (sym) {
        if (sym->type == MECHANISM) {
            /*EMPTY*/
        } else if (sym->type == TEMPLATE && sym->u.ctemplate->is_point_) {
            sym = hoc_table_lookup(name, sym->u.ctemplate->symtable);

        } else {
            sym = 0;
        }
    }
    if (sym) {
        Prop *p, *p0 = 0, *p1;
        // printf("prop_alloc %s %p type=%d\n", sym->name, sym, sym->subtype);
        // need to do this with no args
        hoc_push_frame(sym, 0);
        p = prop_alloc(&p0, sym->subtype, NULL);
        hoc_pop_frame();
        for (; p0 != p; p0 = p1) {
            p1 = p0->next;
            single_prop_free(p0);
        }
        npi_ = new NrnPropertyImpl(p);
        npi_->del_ = true;
    } else {
        npi_ = NULL;
        hoc_execerror(name, "is not a Mechanism or Point Process");
    }
}

NrnProperty::~NrnProperty() {
    delete npi_;
}

bool NrnProperty::deleteable() {
    return npi_->del_;
}

const char* NrnProperty::name() const {
    return npi_->sym_->name;
}

bool NrnProperty::is_point() const {
    return memb_func[npi_->p_->_type].is_point;
}

int NrnProperty::type() const {
    return npi_->p_->_type;
}

Prop* NrnProperty::prop() const {
    return npi_->p_;
}

Symbol* NrnProperty::first_var() {
    npi_->iterator_ = -1;
    return next_var();
}

bool NrnProperty::more_var() {
    if (npi_->iterator_ >= npi_->sym_->s_varn) {
        return false;
    } else {
        return true;
    }
}

Symbol* NrnProperty::next_var() {
    ++npi_->iterator_;
    if (more_var()) {
        return npi_->sym_->u.ppsym[npi_->iterator_];
    } else {
        return 0;
    }
}

Symbol* NrnProperty::var(int i) {
    return npi_->sym_->u.ppsym[i];
}

int NrnProperty::var_type(Symbol* sym) const {
    return nrn_vartype(sym);
}

bool NrnProperty::assign(Prop* src, Prop* dest, int vartype) {
    assert(vartype != NRNPOINTER);
    if (src && dest && src != dest && src->_type == dest->_type) {
        if (src->ob) {
            Symbol* msym = memb_func[src->_type].sym;
            auto const cnt = msym->s_varn;
            for (int i = 0; i < cnt; ++i) {
                Symbol* sym = msym->u.ppsym[i];
                if (vartype == 0 || nrn_vartype(sym) == vartype) {
                    auto const jmax = hoc_total_array_data(sym, 0);
                    auto const n = sym->u.rng.index;
                    auto* const y = dest->ob->u.dataspace[n].pval;
                    auto* const x = src->ob->u.dataspace[n].pval;
                    for (int j = 0; j < jmax; ++j) {
                        y[j] = x[j];
                    }
                }
            }
        } else {
            if (vartype == 0) {
                assert(dest->param_num_vars() == src->param_num_vars());
                auto const n = src->param_num_vars();
                for (int i = 0; i < n; ++i) {
                    assert(dest->param_array_dimension(i) == src->param_array_dimension(i));
                    for (auto j = 0; j < src->param_array_dimension(i); ++j) {
                        dest->param(i, j) = src->param(i, j);
                    }
                }
            } else {
                Symbol* msym = memb_func[src->_type].sym;
                auto const cnt = msym->s_varn;
                for (int i = 0; i < cnt; ++i) {
                    Symbol* sym = msym->u.ppsym[i];
                    if (nrn_vartype(sym) == vartype) {
                        auto const jmax = hoc_total_array_data(sym, 0);
                        auto const n = sym->u.rng.index;
                        assert(src->param_size() == dest->param_size());
                        for (int j = 0; j < jmax; ++j) {
                            dest->param_legacy(n + j) = src->param_legacy(n + j);
                        }
                    }
                }
            }
        }
        return true;
    } else {
        return false;
    }
}
Symbol* NrnProperty::find(const char* name) {
    Symbol* sym;
    int i, cnt;
    cnt = npi_->sym_->s_varn;
    for (i = 0; i < cnt; ++i) {
        sym = npi_->sym_->u.ppsym[i];
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return 0;
}
int NrnProperty::prop_index(const Symbol* s) const {
    assert(s);
    if (s->type != RANGEVAR && s->type != RANGEOBJ) {
        hoc_execerror(s->name, "not a range variable");
    }
    return s->u.rng.index;
}

neuron::container::data_handle<double> NrnProperty::prop_pval(const Symbol* s, int index) const {
    if (npi_->p_->ob) {
        return neuron::container::data_handle<double>{
            npi_->p_->ob->u.dataspace[prop_index(s)].pval + index};
    } else {
        if (s->subtype == NRNPOINTER) {
            return static_cast<neuron::container::data_handle<double>>(
                npi_->p_->dparam[prop_index(s) + index]);
        } else {
            return npi_->p_->param_handle_legacy(prop_index(s) + index);
        }
    }
}

//--------------------------------------------------
SectionList::SectionList(Object* ob) {
    sli_ = new SectionListImpl();
    check_obj_type(ob, "SectionList");
    sli_->ob_ = ob;
    ++ob->refcount;
    sli_->list_ = (struct hoc_Item*) ob->u.this_pointer;
    sli_->itr_ = sli_->list_;
}
SectionList::~SectionList() {
    hoc_dec_refcount(&sli_->ob_);
    delete sli_;
}
Section* SectionList::begin() {
    sli_->itr_ = sli_->list_->next;
    return next();
}
Section* SectionList::next() {
    Section* sec;
    if (sli_->itr_ == sli_->list_) {
        sec = NULL;
    } else {
        sec = hocSEC(sli_->itr_);
        sli_->itr_ = sli_->itr_->next;
    }
    return sec;
}

Object* SectionList::nrn_object() {
    Object* ob = sli_->ob_;
    return ob;
}

#if 0
//---------------------------------------------------
NrnSection::NrnSection(Section* sec) {
	npi_ = new NrnSectionImpl(sec);
}
NrnSection::~NrnSection() {
	delete npi_;
}
void NrnSection::section(Section* sec) {
	section_ref(sec);
	section_unref(npi_->sec_);
	npi_->sec_ = sec;
}
Section* NrnSection::section() {
	return npi_->sec_;
}

Node* NrnSection::node(int inode) { return sec_->pnode[inode]; }

bool NrnSection::is_mechanism(type) {
	return nrn_mechanism(type, node(0)) != (Prop*)0
}

double* NrnSection::var_pointer(const char* var) {
	nrn_pushsec(nsi_->sec_);
	double* pval = hoc_val_pointer(var);
	nrn_popsec();
	return pval;
}
#endif

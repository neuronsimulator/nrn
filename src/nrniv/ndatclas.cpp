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

/*
NrnProperty wrapping a Prop of type EXTRACELL has a problem with vext state
variables because those are not in the Prop.param array but in Node.ExtNode.
At the moment, knowing Prop* does not help with Node*. Note that
Node.ExtNode is not allocated by extcell_alloc(Prop*) but by
extcell_2d_alloc(Section*) when extracellular is inserted.
So, if NrnProperty is wrapping a "real" Prop, it is difficult to find
the Node to determine a pointer to vext[i].
And if NrnProperty is just wrapping a NrnProperty allocated Prop,
the vext states are not allocated. In most cases the issue was avoided
because the "user" of NrnProperty knew the Node*

It is attractive to get rid of NrnProperty altogether. It is mostly used
for iterating over mechanism variables and there are already fairly direct
methods to do that. However, NrnProperty, since it calls the mechanism
allocation method, provides default PARAMETER values. Apart from the
issue of default PARAMETER values, NrnProperty could just wrap a
std::vector<double> param with a length that included PARAMETER, ASSIGNED,
and STATE (all the other defaults are 0.0). The advantage over
allocating a Prop is that the latter includes the extraneous
dparam as well as the full SoA and DataHandle apparatus. We could thread
the needle by allocating a Prop, copying the default PARAMETERS to the
std::vector, and destroying the Prop. Alternatively, the temporary Prop
allocation could be avoided by providing an additional mechanism method, e.g.
mech_param_defaults(mech_type, double* param) fill values starting at param
and avoid the side effect of SoA allocation (which increments
v_structure_change).

*/

//----------------------------------------------------
/* static */ class NrnPropertyImpl {
    friend class NrnProperty;
    NrnPropertyImpl(int mechtype);
    ~NrnPropertyImpl();


    int iterator_;
    int type_;
    Symbol* sym_;
    std::vector<double> params_{};
};
NrnPropertyImpl::NrnPropertyImpl(int mechtype) {
    iterator_ = -1;
    type_ = mechtype;
    sym_ = memb_func[mechtype].sym;
    params_.resize(nrn_prop_param_size_[type_]);
    auto& param_default = *memb_func[mechtype].parm_default;
    int ix = 0;
    int i = 0;
    for (auto val: param_default) {
        Symbol* s = sym_->u.ppsym[i];
        int n = hoc_total_array_data(s, nullptr);
        for (int k = 0; k < n; ++k) {
            params_[ix++] = val;
        }
        ++i;
    }
}

NrnPropertyImpl::~NrnPropertyImpl() {
    // printf("NrnProperty freed prop\n");
}

//---------------------------------------------------
/* static */ class SectionListImpl {
    friend class SectionList;
    Object* ob_;
    struct hoc_Item* itr_;
    struct hoc_Item* list_;
};

//----------------------------------------------------
NrnProperty::NrnProperty(int mechtype) {
    npi_ = new NrnPropertyImpl(mechtype);
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
        npi_ = new NrnPropertyImpl(sym->subtype);
    } else {
        npi_ = NULL;
        hoc_execerror(name, "is not a Mechanism or Point Process");
    }
}

NrnProperty::~NrnProperty() {
    if (npi_) {
        delete npi_;
    }
}

const char* NrnProperty::name() const {
    return npi_->sym_->name;
}

bool NrnProperty::is_point() const {
    return memb_func[npi_->type_].is_point;
}

int NrnProperty::type() const {
    return npi_->type_;
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

bool NrnProperty::copy_out(Prop* dest, Node* nd_dest, int vartype) {
    assert(vartype != NRNPOINTER);
    auto& x = npi_->params_;
    Prop* p = dest;
    if (p && npi_->type_ == p->_type) {
        if (p->ob) {
            Symbol* msym = memb_func[p->_type].sym;
            auto const cnt = msym->s_varn;
            int k = 0;
            for (int i = 0; i < cnt; ++i) {
                Symbol* sym = msym->u.ppsym[i];
                auto const jmax = hoc_total_array_data(sym, 0);
                if (vartype == 0 || nrn_vartype(sym) == vartype) {
                    auto const n = sym->u.rng.index;
                    auto const y = p->ob->u.dataspace[n].pval;
                    for (int j = 0; j < jmax; ++j) {
                        y[j] = x[k + j];
                    }
                }
                k += jmax;
            }
        } else {
            Symbol* msym = memb_func[p->_type].sym;
            auto const cnt = msym->s_varn;
            int k = 0;
            for (int i = 0; i < cnt; ++i) {
                Symbol* sym = msym->u.ppsym[i];
                auto const jmax = hoc_total_array_data(sym, 0);
                if (vartype == 0 || nrn_vartype(sym) == vartype) {
                    auto const n = sym->u.rng.index;
                    for (int j = 0; j < jmax; ++j) {
                        p->param_legacy(n + j) = x[k + j];
                    }
                }
            }
        }
        return true;
    } else {
        return false;
    }
}

bool NrnProperty::copy_in(Prop* src, Node* nd_src, int vartype) {
    return false;
}

bool NrnProperty::copy_out(NrnProperty& np, int vartype) {
    auto& psrc = npi_->params_;
    auto& pdest = np.npi_->params_;
    pdest = psrc;
    return true;
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
    return nullptr;
}

int NrnProperty::prop_index(const Symbol* s) {
    assert(s);
    if (s->type != RANGEVAR) {
        hoc_execerror(s->name, "not a range variable");
    }
    return s->u.rng.index;
}

neuron::container::data_handle<double> NrnProperty::pval(const Symbol* s, int index) {
    double* raw = &npi_->params_[prop_index(s) + index];
    return static_cast<neuron::container::data_handle<double>>(raw);
}

neuron::container::data_handle<double> NrnProperty::prop_pval(Prop* p,
                                                              Node* nd,
                                                              const Symbol* s,
                                                              int index) {
    int sym_index = prop_index(s);
    if (p->ob) {
        return neuron::container::data_handle<double>{p->ob->u.dataspace[sym_index].pval + index};
    } else {
        if (s->subtype == NRNPOINTER) {
            return static_cast<neuron::container::data_handle<double>>(
                p->dparam[sym_index + index]);
        } else {
            if (p->_type == EXTRACELL && sym_index == neuron::extracellular::vext_pseudoindex()) {
                // vext is in Node.ExtNode.v
                return static_cast<neuron::container::data_handle<double>>(nd->extnode->v + index);
            } else {
                return p->param_handle_legacy(sym_index + index);
            }
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

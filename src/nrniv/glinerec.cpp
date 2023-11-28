#include <../../nrnconf.h>
// we want to plot correctly during the local step method when
// cvode.solve(tstop) is invoked. However, not all GraphLine  are time graphs,
// so we assume that the time graphs will be added during stdinit
// during iteration over the graph lists of the stdrun implementation which
// calls Graph.simgraph(). Constantly updating during stdinit
// overcomes the problem of new addexpr, addval, and changed locations of
// point processes.
// Note: cvode.simgraph_remove() deletes all the GLineRecord instances.

#include "hocparse.h"
#include "code.h"
#undef begin
#undef add

#include "nrnoc2iv.h"
#include "vrecitem.h"
#include "netcvode.h"
#include "cvodeobj.h"
#include "utils/enumerate.h"

#if HAVE_IV  // to end of file
#include "graph.h"
#include "glinerec.h"
#include "ocjump.h"

extern NetCvode* net_cvode_instance;

static std::vector<GLineRecord*>* grl;

// Since GraphLine is not an observable, its destructor calls this.
// So ivoc will work, a stub is placed in ivoc/datapath.cpp
void graphLineRecDeleted(GraphLine* gl) {
    if (!grl) {
        return;
    }
    for (auto& item: *grl) {
        if (item->uses(gl)) {
            delete item;
            return;
        }
    }
}

void NetCvode::simgraph_remove() {
    if (!grl) {
        return;
    }
    for (auto& item: *grl) {
        delete item;
    }
}

void Graph::simgraph() {
    if (!grl) {
        grl = new std::vector<GLineRecord*>();
    }
    for (auto& gl: line_list_) {
        PlayRecord* pr = net_cvode_instance->playrec_uses(gl);
        if (pr) {
            delete pr;
        }
        GLineRecord* r = new GLineRecord(gl);
        grl->push_back(r);
    }
}

void GLineRecord::fill_pd1() {
    Inst* pcsav = hoc_pc;
    for (hoc_pc = gl_->expr_->u.u_proc->defn.in;
         hoc_pc->in != STOP;) {  // run machine analogous to hoc_execute(inst);
        double* pd = NULL;
        Inst* pc1 = hoc_pc++;  // must be incremented before the function return.
        if (pc1->pf == rangepoint) {
            hoc_pushx(0.5);  // arc position
            rangevarevalpointer();
            pd = hoc_pxpop();
            hoc_pushx(*pd);
        } else if (pc1->pf == rangevareval) {
            rangevarevalpointer();
            pd = hoc_pxpop();
            hoc_pushx(*pd);
        } else if (pc1->pf == varpush) {
            Symbol* sym = hoc_pc->sym;
            if (strcmp(sym->name, "t") == 0) {
                saw_t_ = true;
            }
            hoc_varpush();
        } else {
            (*((pc1)->pf))();
        }
        if (pd) {
            pd_and_vec_.push_back(std::pair<double*, IvocVect*>(pd, NULL));
        }
    }
    hoc_pc = pcsav;

#if 0
  Symbol* esym = gl_->expr_;
  int sz = esym->u.u_proc->size;
  Inst* inst = esym->u.u_proc->defn.in;
  printf("\n%s\n", gl_->name());
  for (int i = 0; i < sz; ++i) {
    hoc_debugzz(inst + i);
  }
#endif
}

void GLineRecord::fill_pd() {
    // Get rid of old pd_and_vec_ info.
    for (auto& [_, elem]: pd_and_vec_) {
        if (elem) {
            delete elem;
        }
    }
    pd_and_vec_.resize(0);
    saw_t_ = false;
    pd_ = gl_->pval_;
    if (pd_) {
        return;
    }

    // Execute the expr Inst by Inst but when rangepoint or
    // rangevareval are seen, execute a series of stack machine instructions
    // that give us the pointer to the range variable (see the implementation
    // in nrn/src/nrnoc/cabcode.cpp) but leaving the stack as though the
    // original instruction was executed.
    assert(gl_->expr_);
    ObjectContext objc(gl_->obj_);
    fill_pd1();
}

GLineRecord::GLineRecord(GraphLine* gl)
    : PlayRecord({}) {
    // shouldnt be necessary but just in case
    //	printf("GLineRecord %p name=%s\n", this, gl->name());
    gl_ = gl;
    gl_->simgraph_activate(true);
    v_ = NULL;
    saw_t_ = false;
}

GVectorRecord::GVectorRecord(GraphVector* gv)
    : PlayRecord({})
    , gv_{gv} {}

void GraphVector::record_install() {
    new GVectorRecord(this);
}

void GraphVector::record_uninstall() {}

GLineRecord::~GLineRecord() {
    if (v_) {
        delete v_;
        v_ = nullptr;
    }

    for (auto& [_, vec]: pd_and_vec_) {
        if (vec) {
            delete vec;
        }
    }

    if (auto it = std::find(grl->rbegin(), grl->rend(), this); it != grl->rend()) {
        gl_->simgraph_activate(false);
        grl->erase(std::next(it).base());  // Reverse iterator need that
    }
}

GVectorRecord::~GVectorRecord() {}

void GLineRecord::record_init() {
    gl_->simgraph_init();
}

void GVectorRecord::record_init() {}

void GLineRecord::continuous(double t) {
    gl_->simgraph_continuous(t);
}

void GVectorRecord::continuous(double t) {}

int GVectorRecord::count() {
    return gv_->py_data()->count();
}
neuron::container::data_handle<double> GVectorRecord::pdata(int i) {
    return gv_->py_data()->p(i);
}

void GLineRecord::plot(int vecsz, double tstop) {
    double dt = tstop / double(vecsz - 1);
    DataVec* x = (DataVec*) gl_->x_data();
    DataVec* y = (DataVec*) gl_->y_data();
    if (v_) {
        v_->resize(vecsz);
        double* v = vector_vec(v_);
        for (int i = 0; i < vecsz; ++i) {
            x->add(dt * i);
            y->add(v[i]);
        }
    } else if (gl_->expr_) {
        // transfer elements to range variables and plot expression result
        ObjectContext obc(NULL);
        for (int i = 0; i < vecsz; ++i) {
            x->add(dt * i);
            for (auto& [pd, elem]: pd_and_vec_) {
                *pd = elem->elem(i);
            }
            gl_->plot();
        }
    } else {
        assert(0);
    }
}
#else   // do not HAVE_IV
void NetCvode::simgraph_remove() {}
#endif  // HAVE_IV

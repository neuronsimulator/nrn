#include <../../nrnconf.h>
#include <stdio.h>
#include <InterViews/observe.h>
#include "ocnotify.h"
#if HAVE_IV
#include "ivoc.h"
#endif
#include "classreg.h"
#include "linmod.h"
#include "nrnoc2iv.h"

// hoc interface to a LinearModelAddition
// remember that the policy for equation additions to the tree matrix is
// cmat*y' + gmat*y = b and where the first nnode rows specify
// the number of equations (identfied by nodes)
// which are added to existing node equations in the tree structure
// and the first nnode columns specify the voltages (identified by nodes)
// which are coupled to the equations. i.e the number of new equations
// and states added to the tree matrix is nrow - nnode


class LinearMechanism: public Observer {
  public:
    LinearMechanism();
    virtual ~LinearMechanism();
    virtual void disconnect(Observable*);
    virtual void update(Observable*);
    void create();
    void lmfree();
    bool valid() {
        return model_ != NULL;
    }

    LinearModelAddition* model_;
    Matrix* c_;
    Matrix* g_;
    Vect* y_;
    Vect* y0_;
    Vect* b_;
    int nnode_;
    Object* f_callable_;
    Node** nodes_;
    Vect* elayer_;
};

static double valid(void* v) {
    return double(((LinearMechanism*) v)->valid());
}

// dforce(bdot_vector)
// dforce(python_callable, bdot_vector)
// Optional analytic db/dt for IDA mode-3 free y' (Plan A4). Callable is
// invoked at IC with t set to the IC time; it should fill bdot_vector.
static double dforce(void* v) {
    LinearMechanism* m = (LinearMechanism*) v;
    if (!m->model_) {
        return 0.;
    }
    Object* callable = nullptr;
    Vect* bdot = nullptr;
    if (!ifarg(1)) {
        m->model_->set_dforce(nullptr, nullptr);
        return 0.;
    }
    if (hoc_is_object_arg(1)) {
        Object* o = *hoc_objgetarg(1);
        if (o && o->ctemplate && strcmp(o->ctemplate->sym->name, "PythonObject") == 0) {
            callable = o;
            if (!ifarg(2) || !is_vector_arg(2)) {
                hoc_execerror("LinearMechanism.dforce: need dforce(callable, bdot_Vector)", 0);
            }
            bdot = vector_arg(2);
        } else if (is_vector_arg(1)) {
            bdot = vector_arg(1);
        } else {
            hoc_execerror("LinearMechanism.dforce: arg must be Vector or (callable, Vector)", 0);
        }
    } else {
        hoc_execerror("LinearMechanism.dforce: arg must be Vector or (callable, Vector)", 0);
    }
    m->model_->set_dforce(callable, bdot);
    return 1.;
}

static Member_func members[] = {{"valid", valid}, {"dforce", dforce}, {nullptr, nullptr}};

static void* cons(Object*) {
    LinearMechanism* m = new LinearMechanism();
    m->create();
    return (void*) m;
}

static void destruct(void* v) {
    LinearMechanism* m = (LinearMechanism*) v;
    delete m;
}

void LinearMechanism_reg() {
    class2oc("LinearMechanism", cons, destruct, members, nullptr, nullptr);
}

LinearMechanism::LinearMechanism() {
    model_ = NULL;
    c_ = NULL;
    g_ = NULL;
    y_ = NULL;
    b_ = NULL;
    nnode_ = 0;
    nodes_ = NULL;
    y0_ = NULL;
    elayer_ = NULL;
    f_callable_ = NULL;
}

LinearMechanism::~LinearMechanism() {
    // printf("~LinearMechanism\n");
    lmfree();
}

void LinearMechanism::lmfree() {
    if (f_callable_) {
        hoc_obj_unref(f_callable_);
        f_callable_ = NULL;
    }
    if (model_) {
        delete model_;
        model_ = NULL;
    }
    if (nodes_) {
        nrn_notify_pointer_disconnect(this);
        nnode_ = 0;
        delete[] nodes_;
        nodes_ = NULL;
        elayer_ = NULL;
    }
}

void LinearMechanism::disconnect(Observable*) {}
void LinearMechanism::update(Observable*) {
    lmfree();
}

void LinearMechanism::create() {
    int i;
    lmfree();
    i = 0;
    Object* o = *hoc_objgetarg(++i);

    if (strcmp(o->ctemplate->sym->name, "PythonObject") == 0) {
        f_callable_ = o;
        hoc_obj_ref(o);
        c_ = matrix_arg(++i);
    } else {
        f_callable_ = NULL;
        c_ = matrix_arg(1);
    }
    g_ = matrix_arg(++i);
    y_ = vector_arg(++i);

    if (ifarg(i + 2) && hoc_is_object_arg(i + 2) && is_vector_arg(i + 2)) {
        y0_ = vector_arg(++i);
    }
    b_ = vector_arg(++i);
    if (ifarg(++i)) {
#if HAVE_IV
        Oc oc;
#endif
        if (hoc_is_double_arg(i)) {
            nnode_ = 1;
            nodes_ = new Node*[1];
            double x = chkarg(i, 0., 1.);
            Section* sec = chk_access();
            nodes_[0] = node_exact(sec, x);
            neuron::container::notify_when_handle_dies(nodes_[0]->v_handle(), this);
        } else {
            Object* o = *hoc_objgetarg(i);
            check_obj_type(o, "SectionList");
            SectionList* sl = new SectionList(o);
            sl->ref();
            Vect* x = vector_arg(i + 1);
            Section* sec;
            nnode_ = 0;
            nodes_ = new Node*[x->size()];
            for (sec = sl->begin(); sec; sec = sl->next()) {
                nodes_[nnode_] = node_exact(sec, x->elem(nnode_));
                neuron::container::notify_when_handle_dies(nodes_[nnode_]->v_handle(), this);
                ++nnode_;
            }
            if (ifarg(i + 2)) {
                elayer_ = vector_arg(i + 2);
            }
            sl->unref();
        }
    }
    model_ = new LinearModelAddition(c_, g_, y_, y0_, b_, nnode_, nodes_, elayer_, f_callable_);
}

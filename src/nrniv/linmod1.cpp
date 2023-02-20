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

static Member_func members[] = {{"valid", valid}, {0, 0}};

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
    class2oc("LinearMechanism", cons, destruct, members, NULL, NULL, NULL);
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

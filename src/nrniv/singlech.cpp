#include <../../nrnconf.h>
#include <OS/list.h>
#include "nrnoc2iv.h"
#include "ndatclas.h"
#include "ivocvect.h"
#include "ocmatrix.h"
#include "classreg.h"
#include "singlech.h"
#include "membfunc.h"
#include "random1.h"
#include "NegExp.h"

extern "C" {

extern double exprand(double);  // must be changed!

}  // extern "C"
void hoc_reg_singlechan(int, void (*)(...));
void _singlechan_declare(void (*)(double, double*, Datum*), int*, int);
void _nrn_single_react(int, int, double);

/*
encoded in the order of the rates and to_states below is the order
in which the _nrn_single_react statements are made.
*/

class SingleChanState {
  public:
    SingleChanState();
    virtual ~SingleChanState();
    void rate(int to_state, double value);

  public:
    int cond_;

    int n_;
    int size_;
    double* tau_;
    int* to_state_;
};


class SingleChanInfo {
  public:
    int type_;
    void (*f_)(double, double*, Datum*);
    int* slist_;
    int n_;
};

std::vector<SingleChanInfo*> infolist;
static SingleChan* current_chan;

#define NS 3
SingleChanState::SingleChanState() {
    size_ = NS;
    to_state_ = new int[size_];
    tau_ = new double[size_];
    n_ = 0;
    cond_ = 0;
}

SingleChanState::~SingleChanState() {
    delete[] to_state_;
    delete[] tau_;
}

void SingleChanState::rate(int to_state, double value) {
    if (n_ >= size_) {
        int i;
        int size = 2 * size_;
        int* s = new int[size];
        double* r = new double[size];
        for (i = 0; i < size; ++i) {
            s[i] = to_state_[i];
            r[i] = tau_[i];
        }
        delete[] to_state_;
        delete[] tau_;
        to_state_ = s;
        tau_ = r;
        size_ = size;
    }
    to_state_[n_] = to_state;
    tau_[n_] = 1 / value;
    ++n_;
}

void hoc_reg_singlechan(int type, void (*f)(...)) {
    SingleChanInfo* info = new SingleChanInfo();
    info->type_ = type;
    infolist.push_back(info);
    (*f)();
#if 0
	if (nrn_istty_) {
		printf("Allow single channel model for %s\n", memb_func[info->type_].sym->name);
	}
#endif
}

void _singlechan_declare(void (*f)(double, double*, Datum*), int* slist, int n) {
    if (infolist.empty()) {
        return;
    }
    SingleChanInfo* info = infolist.back();
    info->f_ = f;
    info->slist_ = slist;
    info->n_ = n;
}

void _nrn_single_react(int i, int j, double rate) {
    //	printf("_nrn_single_react %d %d %g\n", i, j, rate);
    current_chan->state_[i].rate(j, rate);
}


SingleChan::SingleChan(const char* name) {
    r_ = NULL;
    erand_ = &SingleChan::erand1;
    nprop_ = new NrnProperty(name);
    for (const auto& il: infolist) {
        if (il->type_ == nprop_->type()) {
            info_ = il;
        }
    }
    if (!info_) {
        hoc_execerror(name, "cannot be a SingleChannel");
    }
    state_ = new SingleChanState[info_->n_];
    set_rates(0.);
}

SingleChan::SingleChan(OcMatrix* m) {
    r_ = NULL;
    erand_ = &SingleChan::erand1;
    state_ = NULL;
    nprop_ = NULL;
    info_ = new SingleChanInfo();
    info_->type_ = -1;
    info_->f_ = NULL;
    info_->slist_ = NULL;
    info_->n_ = 0;
    set_rates(m);
}

SingleChan::~SingleChan() {
    if (state_) {
        delete[] state_;
    }
    if (nprop_) {
        delete nprop_;
    } else {
        delete info_;
    }
    if (r_) {
        hoc_obj_unref(r_->obj_);
    }
}
void SingleChan::set_rates(double v) {
    int i, n = info_->n_;
    if (info_->f_) {
        for (i = 0; i < n; ++i) {
            state_[i].n_ = 0;
        }
        current_chan = (SingleChan*) this;
        assert(false);
        //(*info_->f_)(v, nprop_->prop()->param, nprop_->prop()->dparam);
    }
}
void SingleChan::set_rates(OcMatrix* m) {
    assert(nprop_ == NULL);
    if (state_) {
        delete[] state_;
    }
    info_->n_ = m->nrow();
    state_ = new SingleChanState[n()];
    int i, j;
    for (i = 0; i < n(); ++i) {
        SingleChanState& s = state_[i];
        s.n_ = 0;
        for (j = 0; j < n(); ++j) {
            double val = m->getval(i, j);
            if (val > 0.) {
                s.rate(j, 1. / val);
            }
        }
    }
}
void SingleChan::set_rates(int i, int j, double tau) {
    assert(i < n() && j < n() && tau > 0.0);
    SingleChanState& s = state_[i];
    int k;
    for (k = 0; k < n(); ++k) {
        if (j == s.to_state_[k]) {
            s.tau_[k] = tau;
            return;
        }
    }
    assert(k < n());
}

const char* SingleChan::name(int i) {
    return NULL;
}
int SingleChan::index(const char* name) {
    return -1;
}
int SingleChan::cond(int i) {
    return state_[i].cond_;
}
void SingleChan::cond(int i, int cond) {
    state_[i].cond_ = cond;
}
void SingleChan::current_state(int i) {
    current_ = i;
}
int SingleChan::current_state() {
    return current_;
}
int SingleChan::current_cond() {
    return cond(current_);
}

double SingleChan::state_transition() {
    double dt = 1e15;
    double t;
    int i, j = 0, n = state_[current_].n_;
    SingleChanState& s = state_[current_];

    for (i = 0; i < n; ++i) {
        if ((t = s.tau_[i] * (this->*erand_)()) < dt) {
            dt = t;
            j = i;
        }
    }
    current_ = s.to_state_[j];
    return dt;
}

double SingleChan::erand1() {
    return exprand(1);
}
double SingleChan::erand2() {
    return (*r_->rand)();
}
void SingleChan::setrand(Rand* r) {
    if (r) {
        hoc_obj_ref(r->obj_);
        delete r->rand;
        r->rand = new NegativeExpntl(1.0, r->gen);
        erand_ = &SingleChan::erand2;
    } else {
        erand_ = &SingleChan::erand1;
    }
    if (r_) {
        hoc_obj_unref(r_->obj_);
    }
    r_ = r;
}

int SingleChan::n() {
    return info_->n_;
}

double SingleChan::cond_transition() {
    double dt = 0;
    int i = cond(current_);
    do {
        dt += state_transition();
    } while (cond(current_) == i);
    return dt;
}

void SingleChan::state_transitions(Vect* dt, Vect* state) {
    int i;
    int n = dt->size();
    state->resize(n);
    for (i = 0; i < n; ++i) {
        state->elem(i) = current_;
        dt->elem(i) = state_transition();
    }
}

void SingleChan::cond_transitions(Vect* dt, Vect* cond) {
    int i;
    int n = dt->size();
    cond->resize(n);
    for (i = 0; i < n; ++i) {
        cond->elem(i) = current_cond();
        dt->elem(i) = cond_transition();
    }
}

void SingleChan::get_rates(OcMatrix* m) {
    m->resize(n(), n());
    m->zero();
    int i, j;
    for (i = 0; i < n(); ++i) {
        SingleChanState& s = state_[i];
        for (j = 0; j < s.n_; ++j) {
            *m->mep(i, s.to_state_[j]) += 1 / s.tau_[j];
        }
    }
}

static double set_rates(void* v) {
    SingleChan* s = (SingleChan*) v;
    if (hoc_is_object_arg(1)) {
        s->set_rates(matrix_arg(1));
    } else if (ifarg(3)) {
        int i, j;
        double val;
        i = (int) chkarg(1, 0., s->n() - 1);
        j = (int) chkarg(2, 0., s->n() - 1);
        val = chkarg(3, 1e-10, 1e10);
        s->set_rates(i, j, val);
    } else {
        s->set_rates(*getarg(1));
    }
    return 0.;
}

static double get_rates(void* v) {
    SingleChan* s = (SingleChan*) v;
    s->get_rates(matrix_arg(1));
    return 1.;
}

static double set_rand(void* v) {
    SingleChan* s = (SingleChan*) v;
    Object* ob = *hoc_objgetarg(1);
    check_obj_type(ob, "Random");
    Rand* r = (Rand*) (ob->u.this_pointer);
    s->setrand(r);
    return 1.;
}

static double cond(void* v) {
    SingleChan* s = (SingleChan*) v;
    int i = (int) chkarg(1, 0, s->n());
    if (ifarg(2)) {
        s->cond(i, (int) *getarg(2));
    }
    return double(s->cond(i));
}
static double current_state(void* v) {
    SingleChan* s = (SingleChan*) v;
    if (ifarg(1)) {
        s->current_state((int) chkarg(1, 0, s->n()));
    }
    return (double) s->current_state();
}
static double current_cond(void* v) {
    SingleChan* s = (SingleChan*) v;
    return (double) s->cond(s->current_state());
}
static double state_transition(void* v) {
    SingleChan* s = (SingleChan*) v;
    return s->state_transition();
}
static double cond_transition(void* v) {
    SingleChan* s = (SingleChan*) v;
    return s->cond_transition();
}

static double state_transitions(void* v) {
    SingleChan* s = (SingleChan*) v;
    s->state_transitions(vector_arg(1), vector_arg(2));
    return 1.;
}
static double cond_transitions(void* v) {
    SingleChan* s = (SingleChan*) v;
    s->cond_transitions(vector_arg(1), vector_arg(2));
    return 1.;
}

static Member_func members[] = {{"state_transition", state_transition},
                                {"cond_transition", cond_transition},
                                {"set_rates", set_rates},
                                {"get_rates", get_rates},
                                {"cond", cond},
                                {"current_state", current_state},
                                {"current_cond", current_cond},
                                {"state_transitions", state_transitions},
                                {"cond_transitions", cond_transitions},
                                {"set_rand", set_rand},
                                {nullptr, nullptr}};

static void* cons(Object*) {
    SingleChan* s;
    if (hoc_is_str_arg(1)) {
        s = new SingleChan(gargstr(1));
    } else {
        s = new SingleChan(matrix_arg(1));
    }
    return (void*) s;
}

static void destruct(void* v) {
    SingleChan* s = (SingleChan*) v;
    delete s;
}

void SingleChan_reg() {
    class2oc("SingleChan", cons, destruct, members, NULL, NULL, NULL);
}

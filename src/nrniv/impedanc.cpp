#include <../../nrnconf.h>
#undef check
#include "nrniv_mf.h"
#include "nrnmpi.h"
#include "nrn_ansi.h"
#include "nonlinz.h"
#include <InterViews/resource.h>
#include <complex>
#include "nrnoc2iv.h"
#include "classreg.h"
#include <stdio.h>
#include "membfunc.h"
extern void setup_topology();
extern void recalc_diam();

typedef void (*Pfrv4)(int, Node**, double**, Datum**);

class Imp {
  public:
    Imp() = default;
    virtual ~Imp();
    // v(x)/i(x) and  v(loc)/i(x) == v(x)/i(loc)
    int compute(double freq, bool nonlin = false, int maxiter = 500);
    void location(Section*, double);
    double transfer_amp(Section*, double);
    double input_amp(Section*, double);
    double transfer_phase(Section*, double);
    double input_phase(Section*, double);
    double ratio_amp(Section*, double);

  private:
    int loc(Section*, double);
    void alloc();
    void impfree();
    void check();
    void setmat(double);
    void setmat1();

    void LUDecomp();
    void solve();

  public:
    double deltafac_ = .001;

  private:
    int n = 0;
    std::complex<double>* transfer = nullptr;
    std::complex<double>* input = nullptr;
    std::complex<double>* d = nullptr; /* diagonal */
    std::complex<double>* pivot = nullptr;
    int istim = -1; /* where current injected */
    Section* sloc_ = nullptr;
    double xloc_ = 0.;
    NonLinImp* nli_ = nullptr;
};

static void* cons(Object*) {
    Imp* imp = new Imp();
    return (void*) imp;
}

static void destruct(void* v) {
    Imp* imp = (Imp*) v;
    delete imp;
}

static double compute(void* v) {
    Imp* imp = (Imp*) v;
    int rval = 0;
    bool nonlin = false;
    if (ifarg(2)) {
        nonlin = *getarg(2) ? true : false;
    }
    if (ifarg(3)) {
        rval = imp->compute(*getarg(1), nonlin, int(chkarg(3, 1, 1e9)));
    } else {
        rval = imp->compute(*getarg(1), nonlin);
    }
    return double(rval);
}

static double location(void* v) {
    Imp* imp = (Imp*) v;
    double x;
    Section* sec = nullptr;
    if (hoc_is_double_arg(1)) {
        x = chkarg(1, -1., 1.);
        if (x >= 0.0) {
            sec = chk_access();
        }
    } else {
        nrn_seg_or_x_arg(1, &sec, &x);
    }
    imp->location(sec, x);
    return 0.;
}

static double transfer_amp(void* v) {
    Imp* imp = (Imp*) v;
    double x;
    Section* sec;
    nrn_seg_or_x_arg(1, &sec, &x);
    return imp->transfer_amp(sec, x);
}
static double input_amp(void* v) {
    Imp* imp = (Imp*) v;
    double x;
    Section* sec;
    nrn_seg_or_x_arg(1, &sec, &x);
    return imp->input_amp(sec, x);
}
static double transfer_phase(void* v) {
    Imp* imp = (Imp*) v;
    double x;
    Section* sec;
    nrn_seg_or_x_arg(1, &sec, &x);
    return imp->transfer_phase(sec, x);
}
static double input_phase(void* v) {
    Imp* imp = (Imp*) v;
    double x;
    Section* sec;
    nrn_seg_or_x_arg(1, &sec, &x);
    return imp->input_phase(sec, x);
}
static double ratio_amp(void* v) {
    Imp* imp = (Imp*) v;
    double x;
    Section* sec;
    nrn_seg_or_x_arg(1, &sec, &x);
    return imp->ratio_amp(sec, x);
}

static double deltafac(void* v) {
    Imp* imp = (Imp*) v;
    if (ifarg(1)) {
        imp->deltafac_ = chkarg(1, 1e-10, 1);
    }
    return imp->deltafac_;
}

static Member_func members[] = {{"compute", compute},
                                {"loc", location},
                                {"input", input_amp},
                                {"transfer", transfer_amp},
                                {"ratio", ratio_amp},
                                {"input_phase", input_phase},
                                {"transfer_phase", transfer_phase},
                                {"deltafac", deltafac},
                                {0, 0}};

void Impedance_reg() {
    class2oc("Impedance", cons, destruct, members, nullptr, nullptr, nullptr);
}

Imp::~Imp() {
    if (sloc_) {
        section_unref(sloc_);
    }
    impfree();
}

void Imp::impfree() {
    if (d) {
        delete[] d;
        delete[] transfer;
        delete[] input;
        delete[] pivot;
        d = nullptr;
    }
    if (nli_) {
        delete nli_;
        nli_ = nullptr;
    }
}

void Imp::check() {
    NrnThread* _nt = nrn_threads;
    nrn_thread_error("Impedance works with only one thread");
    if (sloc_ && !sloc_->prop) {
        section_unref(sloc_);
        sloc_ = nullptr;
    }
    if (tree_changed) {
        setup_topology();
    }
    if (v_structure_change) {
        recalc_diam();
    }
    if (n != _nt->end) {
        alloc();
    }
}

void Imp::alloc() {
    NrnThread* _nt = nrn_threads;
    impfree();
    n = _nt->end;
    d = new std::complex<double>[n];
    transfer = new std::complex<double>[n];
    input = new std::complex<double>[n];
    pivot = new std::complex<double>[n];
}
int Imp::loc(Section* sec, double x) {
    if (x < 0.0 || sec == nullptr) {
        return -1;
    }
    const Node* nd = node_exact(sec, x);
    return nd->v_node_index;
}

double Imp::transfer_amp(Section* sec, double x) {
    check();
    int vloc = loc(sec, x);
    return nli_ ? nli_->transfer_amp(istim, vloc) : abs(transfer[vloc]);
}

double Imp::input_amp(Section* sec, double x) {
    check();
    return nli_ ? nli_->input_amp(loc(sec, x)) : abs(input[loc(sec, x)]);
}

double Imp::transfer_phase(Section* sec, double x) {
    check();
    return nli_ ? nli_->transfer_phase(istim, loc(sec, x)) : arg(transfer[loc(sec, x)]);
}

double Imp::input_phase(Section* sec, double x) {
    check();
    return nli_ ? nli_->input_phase(loc(sec, x)) : arg(input[loc(sec, x)]);
}

double Imp::ratio_amp(Section* sec, double x) {
    check();
    int i = loc(sec, x);
    return nli_ ? nli_->ratio_amp(i, istim) : (abs(transfer[i] / input[i]));
}

void Imp::location(Section* sec, double x) {
    if (sloc_) {
        section_unref(sloc_);
    }
    sloc_ = sec;
    xloc_ = x;
    if (sloc_) {
        section_ref(sloc_);
    }
}

int Imp::compute(double freq, bool nonlin, int maxiter) {
    int rval = 0;
    check();
    if (sloc_) {
        istim = loc(sloc_, xloc_);
    } else {
        istim = -1;
        if (nrnmpi_numprocs == 0) {
            hoc_execerror("Impedance stimulus location is not specified.", 0);
        }
    }
    if (n == 0 && nrnmpi_numprocs == 1)
        return rval;
    double omega = 1e-6 * 2 * 3.14159265358979323846 * freq;  // wC has units of mho/cm2
    if (nonlin) {
        if (!nli_) {
            nli_ = new NonLinImp();
        }
        nli_->compute(omega, deltafac_, maxiter);
        rval = nli_->solve(istim);
    } else {
        if (nli_) {
            delete nli_;
            nli_ = nullptr;
        }
        if (istim == -1) {
            hoc_execerror("Impedance stimulus location is not specified.", 0);
        }
        setmat(omega);
        LUDecomp();
        solve();
    }
    return rval;
}

void Imp::setmat(double omega) {
    const NrnThread* _nt = nrn_threads;
    setmat1();
    for (int i = 0; i < n; ++i) {
        d[i] = std::complex<double>(NODED(_nt->_v_node[i]), NODERHS(_nt->_v_node[i]) * omega);
        transfer[i] = 0.;
    }
    transfer[istim] = 1.e2 / NODEAREA(_nt->_v_node[istim]);  // injecting 1nA
    // rhs returned is then in units of mV or MegOhms
}


void Imp::setmat1() {
    /*
    The calculated g is good til someone else
    changes something having to do with the matrix.
    */
    auto const sorted_token = nrn_ensure_model_data_are_sorted();
    const NrnThread* _nt = nrn_threads;
    const Memb_list* mlc = _nt->tml->ml;
    assert(_nt->tml->index == CAP);
    for (int i = 0; i < nrn_nthread; ++i) {
        double cj = nrn_threads[i].cj;
        nrn_threads[i].cj = 0;
        // not useful except that many model description set g while computing i
        nrn_rhs(sorted_token, nrn_threads[i]);
        nrn_lhs(sorted_token, nrn_threads[i]);
        nrn_threads[i].cj = cj;
    }
    for (int i = 0; i < n; ++i) {
        NODERHS(_nt->_v_node[i]) = 0;
    }
    for (int i = 0; i < mlc->nodecount; ++i) {
        NODERHS(mlc->nodelist[i]) = mlc->data(i, 0);
    }
}

void Imp::LUDecomp() {
    const NrnThread* _nt = nrn_threads;
    for (int i = _nt->end - 1; i >= _nt->ncell; --i) {
        int ip = _nt->_v_parent[i]->v_node_index;
        pivot[i] = NODEA(_nt->_v_node[i]) / d[i];
        d[ip] -= pivot[i] * NODEB(_nt->_v_node[i]);
    }
}

void Imp::solve() {
    for (int j = 0; j < nrn_nthread; ++j) {
        const NrnThread* _nt = nrn_threads + j;
        for (int i = istim; i >= _nt->ncell; --i) {
            int ip = _nt->_v_parent[i]->v_node_index;
            transfer[ip] -= transfer[i] * pivot[i];
        }
        for (int i = 0; i < _nt->ncell; ++i) {
            transfer[i] /= d[i];
            input[i] = 1. / d[i];
        }
        for (int i = _nt->ncell; i < _nt->end; ++i) {
            int ip = _nt->_v_parent[i]->v_node_index;
            transfer[i] -= NODEB(_nt->_v_node[i]) * transfer[ip];
            transfer[i] /= d[i];
            input[i] = (std::complex<double>(1) + input[ip] * pivot[i] * NODEB(_nt->_v_node[i])) /
                       d[i];
        }
        // take into account area
        for (int i = _nt->ncell; i < _nt->end; ++i) {
            input[i] *= 1e2 / NODEAREA(_nt->_v_node[i]);
        }
    }
}

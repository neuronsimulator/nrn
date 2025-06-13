#include <../../nrnconf.h>
#include <stdio.h>
#include <math.h>
#include <InterViews/resource.h>
#include "nonlinz.h"
#include "nrniv_mf.h"
#include "nrnoc2iv.h"
#include "nrnmpi.h"
#include "membfunc.h"

#include <Eigen/Sparse>
#include <complex>

using namespace std::complex_literals;

extern void v_setup_vectors();
extern int nrndae_extra_eqn_count();
extern Symlist* hoc_built_in_symlist;
extern void (*nrnthread_v_transfer_)(NrnThread*);

extern void pargap_jacobi_rhs(std::vector<std::complex<double>>&,
                              const std::vector<std::complex<double>>&);
extern void pargap_jacobi_setup(int mode);

class NonLinImpRep {
  public:
    NonLinImpRep();
    void delta(double);

    // Functions to fill the matrix
    void didv();
    void dids();
    void dsdv();
    void dsds();

    int gapsolve();

    // Matrix containing the non linear system to solve.
    Eigen::SparseMatrix<std::complex<double>> m_{};
    // The solver of the matrix using the LU decomposition method.
    Eigen::SparseLU<Eigen::SparseMatrix<std::complex<double>>> lu_{};
    int scnt_;  // structure_change
    int n_v_, n_ext_, n_lin_, n_ode_, neq_v_, neq_;
    std::vector<neuron::container::data_handle<double>> pv_, pvdot_;
    std::vector<std::complex<double>> v_;
    std::vector<double> deltavec_;  // just like cvode.atol*cvode.atolscale for ode's
    double delta_;                  // slightly more efficient and easier for v.
    void current(int, Memb_list*, int);
    void ode(int, Memb_list*);

    double omega_;
    int iloc_;  // current injection site of last solve
    float* vsymtol_{};
    int maxiter_{500};
};

NonLinImp::~NonLinImp() {
    delete rep_;
}

double NonLinImp::transfer_amp(int curloc, int vloc) {
    if (nrnmpi_numprocs > 1 && nrnthread_v_transfer_ && curloc != rep_->iloc_) {
        hoc_execerror(
            "current injection site change not allowed with both gap junctions and nhost > 1", 0);
    }
    if (curloc != rep_->iloc_) {
        solve(curloc);
    }
    return std::abs(rep_->v_[vloc]);
}
double NonLinImp::input_amp(int curloc) {
    if (nrnmpi_numprocs > 1 && nrnthread_v_transfer_) {
        hoc_execerror("not allowed with both gap junctions and nhost>1", 0);
    }
    if (curloc != rep_->iloc_) {
        solve(curloc);
    }
    if (curloc < 0) {
        return 0.0;
    }
    return std::abs(rep_->v_[curloc]);
}
double NonLinImp::transfer_phase(int curloc, int vloc) {
    if (nrnmpi_numprocs > 1 && nrnthread_v_transfer_ && curloc != rep_->iloc_) {
        hoc_execerror(
            "current injection site change not allowed with both gap junctions and nhost > 1", 0);
    }
    if (curloc != rep_->iloc_) {
        solve(curloc);
    }
    return std::arg(rep_->v_[vloc]);
}
double NonLinImp::input_phase(int curloc) {
    if (nrnmpi_numprocs > 1 && nrnthread_v_transfer_) {
        hoc_execerror("not allowed with both gap junctions and nhost>1", 0);
    }
    if (curloc != rep_->iloc_) {
        solve(curloc);
    }
    if (curloc < 0) {
        return 0.0;
    }
    return std::arg(rep_->v_[curloc]);
}
double NonLinImp::ratio_amp(int clmploc, int vloc) {
    if (nrnmpi_numprocs > 1 && nrnthread_v_transfer_) {
        hoc_execerror("not allowed with both gap junctions and nhost>1", 0);
    }
    if (clmploc < 0) {
        return 0.0;
    }
    if (clmploc != rep_->iloc_) {
        solve(clmploc);
    }
    return std::abs(rep_->v_[vloc] * std::conj(rep_->v_[clmploc]) / std::norm(rep_->v_[clmploc]));
}
void NonLinImp::compute(double omega, double deltafac, int maxiter) {
    v_setup_vectors();
    nrn_rhs(nrn_ensure_model_data_are_sorted(), nrn_threads[0]);
    if (rep_ && rep_->scnt_ != structure_change_cnt) {
        delete rep_;
        rep_ = nullptr;
    }
    if (!rep_) {
        rep_ = new NonLinImpRep();
    }
    rep_->maxiter_ = maxiter;
    if (rep_->neq_ == 0) {
        return;
    }
    if (nrndae_extra_eqn_count() > 0) {
        hoc_execerror("Impedance calculation with LinearMechanism not implemented", 0);
    }
    if (nrn_threads->_ecell_memb_list) {
        hoc_execerror("Impedance calculation with extracellular not implemented", 0);
    }

    rep_->omega_ = 1000. * omega;
    rep_->delta(deltafac);

    rep_->m_.setZero();

    // fill matrix
    rep_->didv();
    rep_->dsds();
#if 1  // when 0 equivalent to standard method
    rep_->dids();
    rep_->dsdv();
#endif

    // Now that the matrix is filled we can compress it (mandatory for SparseLU)
    rep_->m_.makeCompressed();

    // Factorize the matrix so this is ready to solve
    rep_->lu_.compute(rep_->m_);
    switch (rep_->lu_.info()) {
    case Eigen::Success:
        // Everything fine
        break;
    case Eigen::NumericalIssue:
        hoc_execerror(
            "Eigen Sparse LU factorization failed with Eigen::NumericalIssue, please check the "
            "input matrix:",
            rep_->lu_.lastErrorMessage().c_str());
        break;
    case Eigen::NoConvergence:
        hoc_execerror(
            "Eigen Sparse LU factorization reports Eigen::NonConvergence after calling compute():",
            rep_->lu_.lastErrorMessage().c_str());
        break;
    case Eigen::InvalidInput:
        hoc_execerror(
            "Eigen Sparse LU factorization failed with Eigen::InvalidInput, the input matrix seems "
            "invalid:",
            rep_->lu_.lastErrorMessage().c_str());
        break;
    }

    rep_->iloc_ = -2;
}

int NonLinImp::solve(int curloc) {
    int rval = 0;
    NrnThread* _nt = nrn_threads;
    if (!rep_) {
        hoc_execerror("Must call Impedance.compute first", 0);
    }
    if (rep_->iloc_ != curloc) {
        rep_->iloc_ = curloc;
        rep_->v_ = std::vector<std::complex<double>>(rep_->neq_);
        if (curloc >= 0) {
            rep_->v_[curloc] = 1.e2 / NODEAREA(_nt->_v_node[curloc]);
        }
        if (nrnthread_v_transfer_) {
            rval = rep_->gapsolve();
        } else {
            auto v =
                Eigen::Map<Eigen::Vector<std::complex<double>, Eigen::Dynamic>>(rep_->v_.data(),
                                                                                rep_->v_.size());
            v = rep_->lu_.solve(v);
        }
    }
    return rval;
}

// too bad it is not easy to reuse the cvode/daspk structures. Most of
// mapping is already done there.

NonLinImpRep::NonLinImpRep() {
    NrnThread* _nt = nrn_threads;

    Symbol* vsym = hoc_table_lookup("v", hoc_built_in_symlist);
    if (vsym->extra) {
        vsymtol_ = &vsym->extra->tolerance;
    }
    // the equation order is the same order as for the fixed step method
    // for current balance. Remaining ode equation order is
    // defined by the memb_list.


    // how many equations
    n_v_ = _nt->end;
    n_ext_ = 0;
    if (_nt->_ecell_memb_list) {
        n_ext_ = _nt->_ecell_memb_list->nodecount * nlayer;
    }
    n_lin_ = nrndae_extra_eqn_count();
    n_ode_ = 0;
    for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next) {
        Memb_list* ml = tml->ml;
        int i = tml->index;
        nrn_ode_count_t s = memb_func[i].ode_count;
        if (s) {
            int cnt = (*s)(i);
            n_ode_ += cnt * ml->nodecount;
        }
    }
    neq_v_ = n_v_ + n_ext_ + n_lin_;
    neq_ = neq_v_ + n_ode_;
    if (neq_ == 0) {
        return;
    }
    m_.resize(neq_, neq_);
    pv_.resize(neq_);
    pvdot_.resize(neq_);
    v_.resize(neq_);
    deltavec_.resize(neq_);

    for (int i = 0; i < n_v_; ++i) {
        // utilize nd->eqn_index in case of use_sparse13 later
        Node* nd = _nt->_v_node[i];
        pv_[i] = nd->v_handle();
        pvdot_[i] = nd->rhs_handle();
    }
    scnt_ = structure_change_cnt;
}

void NonLinImpRep::delta(double deltafac) {  // also defines pv_,pvdot_ map for ode
    int i, nc, cnt, ieq;
    NrnThread* nt = nrn_threads;
    for (i = 0; i < neq_; ++i) {
        deltavec_[i] = deltafac;  // all v's wasted but no matter.
    }
    ieq = neq_v_;
    for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
        Memb_list* ml = tml->ml;
        i = tml->index;
        nc = ml->nodecount;
        if (nrn_ode_count_t s = memb_func[i].ode_count; s && (cnt = s(i)) > 0) {
            nrn_ode_map_t ode_map = memb_func[i].ode_map;
            for (auto j = 0; j < nc; ++j) {
                ode_map(ml->prop[j],
                        ieq,
                        pv_.data() + ieq,
                        pvdot_.data() + ieq,
                        deltavec_.data() + ieq,
                        i);
                ieq += cnt;
            }
        }
    }
    delta_ = (vsymtol_ && (*vsymtol_ != 0.)) ? *vsymtol_ : 1.;
    delta_ *= deltafac;
}

void NonLinImpRep::didv() {
    int i, j, ip;
    Node* nd;
    NrnThread* _nt = nrn_threads;
    // d2v/dv2 terms
    for (i = _nt->ncell; i < n_v_; ++i) {
        nd = _nt->_v_node[i];
        ip = _nt->_v_parent[i]->v_node_index;
        m_.coeffRef(ip, i) += NODEA(nd);
        m_.coeffRef(i, ip) += NODEB(nd);
        m_.coeffRef(i, i) -= NODEB(nd);
        m_.coeffRef(ip, ip) -= NODEA(nd);
    }
    // jwC term
    Memb_list* mlc = _nt->tml->ml;
    int n = mlc->nodecount;
    for (i = 0; i < n; ++i) {
        j = mlc->nodelist[i]->v_node_index;
        m_.coeffRef(j, j) += .001 * mlc->data(i, 0) * omega_ * 1i;
    }
    // di/dv terms
    // because there may be several point processes of the same type
    // at the same location, we have to be careful to neither increment that
    // nd->v multiple times nor count the rhs multiple times.
    // So we can't take advantage of vectorized point processes.
    // To avoid this we do each mechanism item separately.
    // We assume there is no interaction between
    // separate locations. Note that interactions such as gap junctions
    // would not be handled in any case without computing a full jacobian.
    // i.e. calling nrn_rhs varying every state one at a time (that would
    // give the d2v/dv2 terms as well), but the expense is unwarranted.
    for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next) {
        i = tml->index;
        if (i == CAP) {
            continue;
        }
        if (!memb_func[i].current) {
            continue;
        }
        Memb_list* ml = tml->ml;
        for (j = 0; j < ml->nodecount; ++j) {
            Node* nd = ml->nodelist[j];
            double x2;
            // zero rhs
            // save v
            NODERHS(nd) = 0;
            double x1 = NODEV(nd);
            // v+dv
            nd->v() = x1 + delta_;
            current(i, ml, j);
            // save rhs
            // zero rhs
            // restore v
            x2 = NODERHS(nd);
            NODERHS(nd) = 0;
            nd->v() = x1;
            current(i, ml, j);
            // conductance
            // add to matrix
            m_.coeffRef(nd->v_node_index, nd->v_node_index) -= (x2 - NODERHS(nd)) / delta_;
        }
    }
}

void NonLinImpRep::dids() {
    // same strategy as didv but now the innermost loop is over
    // every state but just for that compartment
    // outer loop over ode mechanisms in same order as we created the pv_ map	// so we can eas
    int ieq, i, in, is, iis;
    NrnThread* nt = nrn_threads;
    ieq = neq_ - n_ode_;
    for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
        Memb_list* ml = tml->ml;
        i = tml->index;
        if (memb_func[i].ode_count && ml->nodecount) {
            int nc = ml->nodecount;
            nrn_ode_count_t s = memb_func[i].ode_count;
            int cnt = (*s)(i);
            if (memb_func[i].current) {
                for (in = 0; in < ml->nodecount; ++in) {
                    Node* nd = ml->nodelist[in];
                    // zero rhs
                    NODERHS(nd) = 0;
                    // compute rhs
                    current(i, ml, in);
                    // save rhs
                    v_[in].imag(NODERHS(nd));
                    // each state incremented separately and restored
                    for (iis = 0; iis < cnt; ++iis) {
                        is = ieq + in * cnt + iis;
                        // save s
                        v_[is].real(*pv_[is]);
                        // increment s and zero rhs
                        *pv_[is] += deltavec_[is];
                        NODERHS(nd) = 0;
                        current(i, ml, in);
                        *pv_[is] = v_[is].real();  // restore s
                        double g = (NODERHS(nd) - v_[in].imag()) / deltavec_[is];
                        if (g != 0.) {
                            m_.coeffRef(nd->v_node_index, is) = -g;
                        }
                    }
                    // don't know if this is necessary but make sure last
                    // call with respect to original states
                    current(i, ml, in);
                }
            }
            ieq += cnt * nc;
        }
    }
}

void NonLinImpRep::dsdv() {
    int ieq, i, in, is, iis;
    NrnThread* nt = nrn_threads;
    ieq = neq_ - n_ode_;
    for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
        Memb_list* ml = tml->ml;
        i = tml->index;
        if (memb_func[i].ode_count && ml->nodecount) {
            int nc = ml->nodecount;
            nrn_ode_count_t s = memb_func[i].ode_count;
            int cnt = (*s)(i);
            if (memb_func[i].current) {
                // zero rhs, save v
                for (in = 0; in < ml->nodecount; ++in) {
                    Node* nd = ml->nodelist[in];
                    for (is = ieq + in * cnt, iis = 0; iis < cnt; ++iis, ++is) {
                        *pvdot_[is] = 0.;
                    }
                    v_[in].real(NODEV(nd));
                }
                // increment v only once in case there are multiple
                // point processes at the same location
                for (in = 0; in < ml->nodecount; ++in) {
                    Node* nd = ml->nodelist[in];
                    auto const v = nd->v();
                    if (v_[in].real() == v) {
                        nd->v() = v + delta_;
                    }
                }
                // compute rhs. this is the rhs(v+dv)
                ode(i, ml);
                // save rhs, restore v, and zero rhs
                for (in = 0; in < ml->nodecount; ++in) {
                    Node* nd = ml->nodelist[in];
                    for (is = ieq + in * cnt, iis = 0; iis < cnt; ++iis, ++is) {
                        v_[is].imag(*pvdot_[is]);
                        *pvdot_[is] = 0;
                    }
                    nd->v() = v_[in].real();
                }
                // compute the rhs(v)
                ode(i, ml);
                // fill the ds/dv elements
                for (in = 0; in < ml->nodecount; ++in) {
                    Node* nd = ml->nodelist[in];
                    for (is = ieq + in * cnt, iis = 0; iis < cnt; ++iis, ++is) {
                        double ds = (v_[is].imag() - *pvdot_[is]) / delta_;
                        if (ds != 0.) {
                            m_.coeffRef(is, nd->v_node_index) = -ds;
                        }
                    }
                }
            }
            ieq += cnt * nc;
        }
    }
}

void NonLinImpRep::dsds() {
    int ieq, i, in, is, iis, ks, kks;
    NrnThread* nt = nrn_threads;
    // jw term
    for (i = neq_v_; i < neq_; ++i) {
        m_.coeffRef(i, i) += omega_ * 1i;
    }
    ieq = neq_v_;
    for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
        Memb_list* ml = tml->ml;
        i = tml->index;
        if (memb_func[i].ode_count && ml->nodecount) {
            int nc = ml->nodecount;
            nrn_ode_count_t s = memb_func[i].ode_count;
            int cnt = (*s)(i);
            // zero rhs, save s
            for (in = 0; in < ml->nodecount; ++in) {
                for (is = ieq + in * cnt, iis = 0; iis < cnt; ++iis, ++is) {
                    *pvdot_[is] = 0.;
                    v_[is].real(*pv_[is]);
                }
            }
            // compute rhs. this is the rhs(s)
            ode(i, ml);
            // save rhs
            for (in = 0; in < ml->nodecount; ++in) {
                for (is = ieq + in * cnt, iis = 0; iis < cnt; ++iis, ++is) {
                    v_[is].imag(*pvdot_[is]);
                }
            }
            // iterate over the states
            for (kks = 0; kks < cnt; ++kks) {
                // zero rhs, increment s(ks)
                for (in = 0; in < ml->nodecount; ++in) {
                    ks = ieq + in * cnt + kks;
                    for (is = ieq + in * cnt, iis = 0; iis < cnt; ++iis, ++is) {
                        *pvdot_[is] = 0.;
                    }
                    *pv_[ks] += deltavec_[ks];
                }
                ode(i, ml);
                // store element and restore s
                // fill the ds/dv elements
                for (in = 0; in < ml->nodecount; ++in) {
                    Node* nd = ml->nodelist[in];
                    ks = ieq + in * cnt + kks;
                    for (is = ieq + in * cnt, iis = 0; iis < cnt; ++iis, ++is) {
                        double ds = (*pvdot_[is] - v_[is].imag()) / deltavec_[is];
                        if (ds != 0.) {
                            m_.coeffRef(is, ks) = -ds;
                        }
                        *pv_[ks] = v_[ks].real();
                    }
                }
                // perhaps not necessary but ensures the last computation is with
                // the base states.
                ode(i, ml);
            }
            ieq += cnt * nc;
        }
    }
}

void NonLinImpRep::current(int im, Memb_list* ml, int in) {  // assume there is in fact a current
                                                             // method
    // fake a 1 element memb_list
    Memb_list mfake{im};
    mfake.nodeindices = ml->nodeindices + in;
    mfake.nodelist = ml->nodelist + in;
    mfake.set_storage_offset(ml->get_storage_offset());
    mfake.pdata = ml->pdata + in;
    mfake.prop = ml->prop ? ml->prop + in : nullptr;
    mfake.nodecount = 1;
    mfake._thread = ml->_thread;
    memb_func[im].current(nrn_ensure_model_data_are_sorted(), nrn_threads, &mfake, im);
}

void NonLinImpRep::ode(int im, Memb_list* ml) {  // assume there is in fact an ode method
    memb_func[im].ode_spec(nrn_ensure_model_data_are_sorted(), nrn_threads, ml, im);
}

// This function compute a solution of a converging system by iteration.
// The value returned is the number of iterations to reach a precision of "tol" (1e-9).
int NonLinImpRep::gapsolve() {
    // On entry, v_ contains the complex b for A*x = b.
    // On return v_ contains complex solution, x.
    // m_ is the factored matrix for the trees without gap junctions
    // Jacobi method (easy for parallel)
    // A = D + R
    // D*x_(k+1) = (b - R*x_(k))
    // D is m_ (and includes the gap junction contribution to the diagonal)
    // R is the off diagonal matrix of the gaps.

    // one and only one stimulus
#if NRNMPI
    if (nrnmpi_numprocs > 1 && nrnmpi_int_sum_reduce(iloc_ >= 0 ? 1 : 0) != 1) {
        if (nrnmpi_myid == 0) {
            hoc_execerror("there can be one and only one impedance stimulus", 0);
        }
    }
#endif

    pargap_jacobi_setup(0);  // 0 means 'setup'

    std::vector<std::complex<double>> x_old(neq_);
    std::vector<std::complex<double>> x(neq_);
    std::vector<std::complex<double>> b(v_);

    // iterate till change in x is small
    double tol = 1e-9;
    double delta{};

    int success = 0;
    int iter;

    for (iter = 1; iter <= maxiter_; ++iter) {
        if (neq_) {
            auto b_ = Eigen::Map<Eigen::Vector<std::complex<double>, Eigen::Dynamic>>(b.data(),
                                                                                      b.size());
            auto x_ = Eigen::Map<Eigen::Vector<std::complex<double>, Eigen::Dynamic>>(x.data(),
                                                                                      x.size());
            x_ = lu_.solve(b_);
        }

        // if any change in x > tol, then do another iteration.
        success = 1;
        delta = 0.0;
        // Do the substraction of the previous result (x_old) and current result (x).
        // If all differences are < tol stop the loop, otherwise continue to iterate
        for (int i = 0; i < neq_; ++i) {
            auto diff = x[i] - x_old[i];
            double err = std::abs(diff.real()) + std::abs(diff.imag());
            if (err > tol) {
                success = 0;
            }
            delta = std::max(err, delta);
        }
#if NRNMPI
        if (nrnmpi_numprocs > 1) {
            success = nrnmpi_int_sum_reduce(success) / nrnmpi_numprocs;
        }
#endif
        if (success) {
            v_ = x;
            break;
        }

        // setup for next iteration
        x_old = x;
        b = v_;
        pargap_jacobi_rhs(b, x_old);
    }

    pargap_jacobi_setup(1);  // 1 means 'tear down'

    if (!success) {
        char buf[256];
        Sprintf(buf,
                "Impedance calculation did not converge in %d iterations. Max state change on last "
                "iteration was %g (Iterations stop at %g)\n",
                maxiter_,
                delta,
                tol);
        hoc_execerror(buf, nullptr);
    }
    return iter;
}

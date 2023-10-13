#ifndef kschan_h
#define kschan_h

#include <math.h>
#include "nrnoc2iv.h"
#include "ivocvect.h"
#include "nrnunits.h"

#include "spmatrix.h"

// extern double dt;
extern double celsius;

class KSState;
class KSChan;
class KSSingle;
struct NrnThread;

class KSChanFunction {
  public:
    KSChanFunction();
    virtual ~KSChanFunction();
    virtual int type() {
        return 0;
    }
    virtual double f(double v) {
        return 1.;
    }
    void f(int cnt, double* v, double* val) {
        int i;
        for (i = 0; i < cnt; ++i) {
            val[i] = f(v[i]);
        }
    }
    static KSChanFunction* new_function(int type, Vect*, double, double);
    Vect* gp_;  // gate parameters
    double c(int i) {
        return gp_->elem(i);
    }
    static double Exp(double x) {
        if (x > 700.) {
            return exp(700.);
        } else if (x < -700.) {
            return exp(-700.);
        } else {
            return exp(x);
        }
    }
};

class KSChanConst: public KSChanFunction {
  public:
    virtual int type() {
        return 1;
    }
    virtual double f(double v) {
        return c(0);
    }
};

class KSChanExp: public KSChanFunction {
  public:
    virtual int type() {
        return 2;
    }
    virtual double f(double v) {
        return c(0) * Exp(c(1) * (v - c(2)));
    }
};

class KSChanLinoid: public KSChanFunction {
  public:
    virtual int type() {
        return 3;
    }
    virtual double f(double v) {
        double x = c(1) * (v - c(2));
        if (fabs(x) > 1e-6) {
            return c(0) * x / (1 - Exp(-x));
        } else {
            return c(0) * (1 + x / 2.);
        }
    }
};

class KSChanSigmoid: public KSChanFunction {
  public:
    virtual int type() {
        return 4;
    }
    virtual double f(double v) {
        return c(0) / (1. + Exp(c(1) * (v - c(2))));
    }
};


#define ebykt (_e_over_k_codata2018 / (273.15 + celsius))

// from MODELING NEURONAL BIOPHYSICS Lyle J Graham
// A Chapter in the Handbook of Brain Theory and Neural Networks, Volume 2
// a' = K*exp(z*gam*(v-vhalf)*F/RT)
// b' = K*exp(-z*(1-gam)*(v-vhalf)*F/RT)
// but tau = 1/(a' + b') + tau0
// and inf = a'/(a' + b')
// so there is no fast way to get either a,b or inf,tau individually

class KSChanBGinf: public KSChanFunction {
  public:
    virtual int type() {
        return 5;
    }
    virtual double f(double v) {
        double x = ebykt * c(2) * (v - c(1));
        double ap = c(0) * Exp(c(3) * x);
        double bp = c(0) * Exp((c(3) - 1.) * x);
        tau = 1 / (ap + bp);
        double inf = ap * tau;
        tau += c(4);
        return inf;
    }
    double tau;  // may avoid duplicate call to KSChanBGtau
};

class KSChanBGtau: public KSChanFunction {
  public:
    virtual int type() {
        return 6;
    }
    virtual double f(double v) {
        double x = ebykt * c(2) * (v - c(1));
        double ap = c(0) * Exp(c(3) * x);
        double bp = c(0) * Exp((c(3) - 1.) * x);
        double tau = 1 / (ap + bp);
        inf = ap * tau;
        tau += c(4);
        return tau;
    }
    double inf;  // may avoid duplicate call to KSChanBGinf
};

class KSChanTable: public KSChanFunction {
  public:
    KSChanTable(Vect*, double vmin, double vmax);
    virtual int type() {
        return 7;
    }
    virtual double f(double v);
    double vmin_, vmax_;

  private:
    double dvinv_;
};

class KSTransition {
  public:
    KSTransition();
    virtual ~KSTransition();
    // vmin, vmax only for type KSChanTable
    void setf(int direction, int type, Vect* vec, double vmin, double vmax);
    // only voltage gated for now
    double alpha(double v) {
        return type_ == 0 ? f0->f(v) : f0->f(v) / f1->f(v);
    }
    double beta(double v) {
        return type_ == 0 ? f1->f(v) : (1. - f0->f(v)) / f1->f(v);
    }
    double inf(double v) {
        return type_ == 1 ? f0->f(v) : f0->f(v) / (f0->f(v) + f1->f(v));
    }
    double tau(double v) {
        return type_ == 1 ? f1->f(v) : 1. / (f0->f(v) + f1->f(v));
    }
    void ab(double v, double& a, double& b);
    void ab(Vect* v, Vect* a, Vect* b);
    void inftau(double v, double& inf, double& tau);
    void inftau(Vect* v, Vect* inf, Vect* tau);
    // hh tables
    // easily out of date!!
    // in anything about f0 or f1 changes then must call hh_table_make;
    void hh_table_make(double dt, int size = 200, double vmin = -100., double vmax = 50.);
    bool usehhtable() {
        return (size1_ > 0);
    }
    void inftau_hh_table(int i, double& inf, double& tau) {
        inf = inftab_[i];
        tau = tautab_[i];
    }
    void inftau_hh_table(int i, double x, double& inf, double& tau) {
        inf = inftab_[i] + (inftab_[i + 1] - inftab_[i]) * x;
        tau = tautab_[i] + (tautab_[i + 1] - tautab_[i]) * x;
    }
    // the agent style
    virtual double alpha(Datum*);
    virtual double beta();
    void lig2pd(int pdoff);

  public:
    Object* obj_;
    int index_;  // into trans_ array
    int src_;
    int target_;
    KSChan* ks_;
    KSChanFunction* f0;
    KSChanFunction* f1;
    int type_;  // 0-ab,voltage gated; 1-inftau voltage gated
                // 2-ligand outside;  3-ligand inside
    int ligand_index_;
    int pd_index_;
    int stoichiom_;

  private:
    // for hh tables.
    double* inftab_;
    double* tautab_;
    int size1_;
};

class KSGateComplex {
  public:
    KSGateComplex();
    virtual ~KSGateComplex();
    double conductance(Memb_list* ml, std::size_t instance, std::size_t offset, KSState* st);

  public:
    Object* obj_;
    KSChan* ks_;
    int index_;   // into gc_ array
    int sindex_;  // starting state index for this complex
    int nstate_;  // number of states
    int power_;   // eg. n^4, or m^3
};

struct KSIv {
    virtual ~KSIv() = default;
    // this one for ionic ohmic and nernst.
    virtual double cur(double g,
                       Datum* pd,
                       double v,
                       Memb_list* ml,
                       std::size_t instance,
                       std::size_t offset);
    virtual double jacob(Datum* pd,
                         double v,
                         Memb_list* ml,
                         std::size_t instance,
                         std::size_t offset);
};
struct KSIvghk: KSIv {
    // this one for ionic Goldman-Hodgkin-Katz
    double cur(double g,
               Datum* pd,
               double v,
               Memb_list* ml,
               std::size_t instance,
               std::size_t offset) override;
    double jacob(Datum* pd,
                 double v,
                 Memb_list* ml,
                 std::size_t instance,
                 std::size_t offset) override;
    double z;
};
struct KSIvNonSpec: KSIv {
    // this one for non-specific ohmic. There will be a PARAMETER e_suffix at p[1]
    double cur(double g,
               Datum* pd,
               double v,
               Memb_list* ml,
               std::size_t instance,
               std::size_t offset) override;
    double jacob(Datum* pd,
                 double v,
                 Memb_list* ml,
                 std::size_t instance,
                 std::size_t offset) override;
};

struct KSPPIv: KSIv {
    // this one for POINT_PROCESS ionic ohmic and nernst.
    double cur(double g,
               Datum* pd,
               double v,
               Memb_list* ml,
               std::size_t instance,
               std::size_t offset) override;
    double jacob(Datum* pd,
                 double v,
                 Memb_list* ml,
                 std::size_t instance,
                 std::size_t offset) override;
    int ppoff_;
};
struct KSPPIvghk: KSPPIv {
    // this one for POINT_PROCESS ionic Goldman-Hodgkin-Katz
    double cur(double g,
               Datum* pd,
               double v,
               Memb_list* ml,
               std::size_t instance,
               std::size_t offset) override;
    double jacob(Datum* pd,
                 double v,
                 Memb_list* ml,
                 std::size_t instance,
                 std::size_t offset) override;
    double z;
};
struct KSPPIvNonSpec: KSPPIv {
    // this one for POINT_PROCESS non-specific ohmic. There will be a PARAMETER e_suffix at p[1]
    double cur(double g,
               Datum* pd,
               double v,
               Memb_list* ml,
               std::size_t instance,
               std::size_t offset) override;
    double jacob(Datum* pd,
                 double v,
                 Memb_list* ml,
                 std::size_t instance,
                 std::size_t offset) override;
};

class KSState {
  public:
    KSState();
    virtual ~KSState();
    const char* string() {
        return name_.c_str();
    }
    double f_;  // normalized conductance
    std::string name_;
    int index_;  // into state_ array
    KSChan* ks_;
    Object* obj_;
};

class KSChan {
  public:
    KSChan(Object*, bool is_point = false);
    virtual ~KSChan() {}
    virtual void alloc(Prop*);
    virtual void init(NrnThread*, Memb_list*);
    virtual void state(NrnThread*, Memb_list*);
    virtual void cur(NrnThread*, Memb_list*);
    virtual void jacob(NrnThread*, Memb_list*);
    void add_channel(const char**);
    // for cvode
    virtual int count();
    virtual void map(Prop*,
                     int,
                     neuron::container::data_handle<double>*,
                     neuron::container::data_handle<double>*,
                     double*);
    virtual void spec(Memb_list*);
    virtual void matsol(NrnThread*, Memb_list*);
    virtual void cv_sc_update(NrnThread*, Memb_list*);
    double conductance(double gmax, Memb_list* ml, std::size_t instance, std::size_t offset);

  public:
    // hoc accessibilty
    const char* state(int index);
    int trans_index(int src, int target);  // index of the transition
    int gate_index(int state_index);       // index of the gate
    bool is_point() {
        return is_point_;
    }
    bool is_single() {
        return is_single_;
    }
    void set_single(bool, bool update = true);
    void destroy_pnt(Point_process*);
    int nsingle(Point_process*);
    void nsingle(Point_process*, int);
    double alpha(double v, int, int) {
        return 0.;
    }
    double beta(double v, int, int) {
        return 0.;
    }
    void setstructure(Vect*);
    void setname(const char*);
    void setsname(int, const char*);
    void setion(const char*);
    void settype(KSTransition*, int type, const char*);
    void setivrelation(int);
    // hoc incremental management
    KSState* add_hhstate(const char*);
    KSState* add_ksstate(int igate, const char*);
    void remove_state(int);
    // these are only for kinetic scheme transitions since an hh
    // always has one and only one transition.
    KSTransition* add_transition(int src, int target);
    void remove_transition(int);
    void setcond();
    void power(KSGateComplex*, int);
    void usetable(bool, int size, double vmin, double vmax);
    void usetable(bool);
    int usetable(double* vmin, double* vmax);  // get info
    bool usetable() {
        return usetable_;
    }
    void check_table_thread(NrnThread*);

  private:
    void free1();
    void setupmat();
    void fillmat(double v, Datum* pd);
    void mat_dt(double dt, Memb_list* ml, std::size_t instance, std::size_t offset);
    void solvemat(Memb_list*, std::size_t instance, std::size_t offset);
    void mulmat(Memb_list* ml, std::size_t instance, std::size_t offset_s, std::size_t offset_ds);
    void ion_consist();
    void ligand_consist(int, int, Prop*, Node*);
    Prop* needion(Symbol*, Node*, Prop*);
    void sname_install();
    Symbol* looksym(const char*, Symbol* tmplt = NULL);
    Symbol* installsym(const char*, int, Symbol* tmplt = NULL);
    void freesym(Symbol*, Symbol* tmplt = NULL);
    Symbol** newppsym(int);
    void delete_schan_node_data();
    void alloc_schan_node_data();
    void update_prop();  // can add and remove Nsingle and SingleNodeData
    void update_size();
    void must_allow_size_update(bool single, bool ion, int ligand, int nstate) const;

    KSState* state_insert(int i, const char* name, double frac);
    void state_remove(int i);
    KSGateComplex* gate_insert(int ig, int is, int power);
    void gate_remove(int i);
    KSTransition* trans_insert(int i, int src, int target);
    void trans_remove(int i);
    void check_struct();
    int state_size_;
    int gate_size_;
    int trans_size_;
    bool is_point_;
    bool is_single_;
    // for point process
    int pointtype_;
    int mechtype_;

  public:
    std::string name_;  // name of channel
    std::string ion_;   // name of ion , "" means non-specific
    double gmax_deflt_;
    double erev_deflt_;
    int cond_model_;
    KSIv* iv_relation_;
    int ngate_;       // number of gating complexes
    int ntrans_;      // total number of transitions
    int ivkstrans_;   // index of beginning of voltage sensitive ks transitions
    int iligtrans_;   // index of beginning of ligand sensitive ks transitions
                      // 0 to ivkstrans_ - 1 are the hh transitions
    int nhhstate_;    // total number of hh states, does not include ks states
    int nksstate_;    // total number of kinetic scheme states.
    int nstate_;      // total number of all states, nhhstate_ to nstate_ - 1
                      // are kinetic scheme states. The nhhstate_ are first
                      // and the nhhstate_ = ivkstrans_
    KSState* state_;  // the state names
    KSGateComplex* gc_;
    KSTransition* trans_;  // array of transitions
    Symbol* ion_sym_;      // if NULL then non-specific and e_suffix is a parameter
    int nligand_;
    Symbol** ligands_;
    Object* obj_;
    KSSingle* single_;

  private:
    int cvode_ieq_;
    Symbol* mechsym_;  // the top level symbol (insert sym or new sym)
    Symbol* rlsym_;    // symbol with the range list (= mechsym_ when  density)
    char* mat_;
    double** elms_;
    double** diag_;
    int dsize_;       // size of prop->dparam
    int psize_;       // size of prop->param
    int soffset_;     // STATE begins here in the p array.
    int gmaxoffset_;  // gmax is here in the p array, normally 0 but if
                      // there is an Nsingle then it is 1.
    int ppoff_;       // 2 or 3 for point process since area and Point_process* are
                      // first two elements and there may be a KSSingleNodeData*
    // for hh rate tables
    double vmin_, vmax_, dvinv_, dtsav_;
    int hh_tab_size_;
    bool usetable_;
};

#endif

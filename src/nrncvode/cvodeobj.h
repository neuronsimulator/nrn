#ifndef cvodeobj_h
#define cvodeobj_h

#include "nrnmpi.h"
#include "nrnneosm.h"
//#include "shared/nvector_serial.h"
#include "shared/nvector.h"
#include "membfunc.h"
#include "netcon.h"

class NetCvode;
class Daspk;
class TQItem;
class TQueue;
typedef std::vector<PreSyn*> PreSynList;
struct BAMech;
struct NrnThread;
class PlayRecord;
class STEList;
class HTList;
namespace neuron {
struct model_sorted_token;
}

/**
 * @brief Wrapper for Memb_list in CVode related code.
 *
 * This gets used in two ways:
 * - with ml.size() == 1 and ml[0].nodecount > 1 when the mechanism instances to be processed are
 *   contiguous
 * - with ml.size() >= 1 and ml[i].nodecount == 1 when non-contiguous instances need to be processed
 *
 * generic configurations with ml.size() and ml[i].nodecount both larger than one are not supported.
 */
struct CvMembList {
    CvMembList(int type)
        : index{type} {
        ml.emplace_back(type);
    }
    CvMembList* next{};
    std::vector<Memb_list> ml{};
    int index{};
};

struct BAMechList {
    BAMechList(BAMechList** first);
    BAMechList* next;
    BAMech* bam;
    std::vector<Memb_list*> ml;
    static void destruct(BAMechList** first);
};

#define CTD(i) ctd_[((nctd_ > 1) ? (i) : 0)]
class CvodeThreadData {
  public:
    CvodeThreadData();
    virtual ~CvodeThreadData();
    void delete_memb_list(CvMembList*);

    int no_cap_count_;  // number of nodes with no capacitance
    int no_cap_child_count_;
    Node** no_cap_node_;
    Node** no_cap_child_;  // connected to nodes that have no capacitance
    CvMembList* cv_memb_list_;
    CvMembList* cmlcap_;
    CvMembList* cmlext_;       // used only by daspk
    CvMembList* no_cap_memb_;  // used only by cvode, point processes in the no cap nodes
    BAMechList* before_breakpoint_;
    BAMechList* after_solve_;
    BAMechList* before_step_;
    int rootnodecount_;
    int v_node_count_;
    Node** v_node_;
    Node** v_parent_;
    PreSynList* psl_th_;  // with a threshold
    HTList* watch_list_;
    std::vector<neuron::container::data_handle<double>> pv_, pvdot_;
    int nvoffset_;              // beginning of this threads states
    int nvsize_;                // total number of states for this thread
    int neq_v_;                 // for daspk, number of voltage states for this thread
    int nonvint_offset_;        // synonym for neq_v_. Beginning of this threads nonvint variables.
    int nonvint_extra_offset_;  // extra states (probably Python). Not scattered or gathered.
    std::vector<PlayRecord*>* play_;
    std::vector<PlayRecord*>* record_;
};

class Cvode {
  public:
    Cvode(NetCvode*);
    Cvode();
    virtual ~Cvode();

    virtual int handle_step(neuron::model_sorted_token const&, NetCvode*, double);
    virtual int init(double t);
    virtual int advance_tn(neuron::model_sorted_token const&);
    virtual int interpolate(double t);
    virtual double tn() {
        return tn_;
    }  // furthest time of advance
    virtual double t0() {
        return t0_;
    }  // beginning of last real step
    void init_prepare();

    int solve();  // checks event_flag and init or advance_tn
    void statistics();
    double gam();
    double time() const {
        return t_;
    }
    void free_cvodemem();
    int order();
    void maxorder(int), minstep(double), maxstep(double);

  public:
    double tn_, t0_, t_;
    bool initialize_;
    bool can_retreat_;  // only true after an integration step
    // statistics
    void stat_init();
    int advance_calls_, interpolate_calls_, init_calls_;
    int f_calls_, mxb_calls_, jac_calls_, ts_inits_;

  private:
    void alloc_cvode();
    void alloc_daspk();
    int cvode_init(double);
    int cvode_advance_tn(neuron::model_sorted_token const&);
    int cvode_interpolate(double);
    int daspk_init(double);
    int daspk_advance_tn();
    int daspk_interpolate(double);

  public:
    N_Vector nvnew(long);
    int setup(N_Vector ypred, N_Vector fpred);
    int solvex_thread(neuron::model_sorted_token const&, double* b, double* y, NrnThread* nt);
    int solvex_thread_part1(double* b, NrnThread* nt);
    int solvex_thread_part2(NrnThread* nt);
    int solvex_thread_part3(double* b, NrnThread* nt);
    void fun_thread(neuron::model_sorted_token const&,
                    double t,
                    double* y,
                    double* ydot,
                    NrnThread* nt);
    void fun_thread_transfer_part1(neuron::model_sorted_token const&,
                                   double t,
                                   double* y,
                                   NrnThread* nt);
    void fun_thread_transfer_part2(neuron::model_sorted_token const&, double* ydot, NrnThread* nt);
    void fun_thread_ms_part1(double t, double* y, NrnThread* nt);
    void fun_thread_ms_part2(NrnThread* nt);
    void fun_thread_ms_part3(NrnThread* nt);
    void fun_thread_ms_part4(double* ydot, NrnThread* nt);
    void fun_thread_ms_part34(double* ydot, NrnThread* nt);
    bool at_time(double, NrnThread*);
    void set_init_flag();
    void check_deliver(NrnThread* nt = 0);
    void evaluate_conditions(NrnThread* nt = 0);
    void ste_check();
    void states(double*);
    void dstates(double*);
    void error_weights(double*);
    void acor(double*);
    void fill(Cvode* standard);
    // following 7 crucial for local time step recording, also used by global
    void delete_prl();
    void record_add(PlayRecord*);
    void record_continuous();
    void record_continuous_thread(NrnThread*);
    void play_add(PlayRecord*);
    void play_continuous(double t);
    void play_continuous_thread(double t, NrnThread*);
    void do_ode(neuron::model_sorted_token const&, NrnThread&);
    void do_nonode(neuron::model_sorted_token const&, NrnThread* nt = 0);
    double* n_vector_data(N_Vector, int);

  private:
    void cvode_constructor();
    bool init_global();
    void init_eqn();
    void daspk_init_eqn();
    void matmeth();
    void nocap_v(neuron::model_sorted_token const&, NrnThread*);
    void nocap_v_part1(NrnThread*);
    void nocap_v_part2(NrnThread*);
    void nocap_v_part3(NrnThread*);
    void solvemem(neuron::model_sorted_token const&, NrnThread*);
    void atolvec_alloc(int);
    double h();
    N_Vector ewtvec();
    N_Vector acorvec();
    void new_no_cap_memb(CvodeThreadData&, NrnThread*);
    void before_after(neuron::model_sorted_token const&, BAMechList*, NrnThread*);

  public:
    // daspk
    bool use_daspk_;
    Daspk* daspk_;
    int res(double, double*, double*, double*, NrnThread*);
    int psol(double, double*, double*, double, NrnThread*);
    void daspk_scatter_y(N_Vector);  // daspk solves vi,vx instead of vm,vx
    void daspk_gather_y(N_Vector);
    void daspk_scatter_y(double*, int);
    void daspk_gather_y(double*, int);
    void scatter_y(neuron::model_sorted_token const&, double*, int);
    void gather_y(N_Vector);
    void gather_y(double*, int);
    void scatter_ydot(double*, int);
    void gather_ydot(N_Vector);
    void gather_ydot(double*, int);

  public:
    void activate_maxstate(bool);
    void maxstate(double*);
    void maxstate(bool, NrnThread* nt = 0);
    void maxacor(double*);

  public:
    void* mem_;
    N_Vector y_;
    N_Vector atolnvec_;
    N_Vector maxstate_;
    N_Vector maxacor_;

  public:
    bool structure_change_;
#if USENEOSIM
    TQueue* neosim_self_events_;
#endif
  public:
    CvodeThreadData* ctd_;
    NrnThread* nth_;  // for lvardt
    int nctd_;
    long int* nthsizes_;  // N_Vector_NrnThread uses this copy of ctd_[i].nvsize_
    NetCvode* ncv_;
    int neq_;
    int event_flag_;
    double next_at_time_;
    double tstop_;
    double tstop_begin_, tstop_end_;

  private:
    void rhs(neuron::model_sorted_token const&, NrnThread*);
    void rhs_memb(neuron::model_sorted_token const&, CvMembList*, NrnThread*);
    void lhs(neuron::model_sorted_token const&, NrnThread*);
    void lhs_memb(neuron::model_sorted_token const&, CvMembList*, NrnThread*);
    void triang(NrnThread*);
    void bksub(NrnThread*);

  private:
    // segregation of old vectorized information to per cell info
    friend class NetCvode;
    bool is_owner(neuron::container::data_handle<double> const&);  // for play and record in
                                                                   // local step context.
    bool local_;
    void daspk_setup1_tree_matrix();  // unused
    void daspk_setup2_tree_matrix();  // unused
    TQItem* tqitem_;

  private:
    int prior2init_;
#if NRNMPI
  public:
    bool use_partrans_;
    int global_neq_;
    int opmode_;  // 1 advance, 2 interpolate, 3 init; for testing
#endif            // NRNMPI
};

#endif

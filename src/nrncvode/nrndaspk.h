#pragma once

#include "shared/nvector_serial.h"
#include "nvector_nrnthread.h"
#include "nvector_nrnthread_ld.h"
#include "nvector_nrnserial_ld.h"
#include "vecplay_tplus.h"
#include <cstdio>
#include <string>
#include <vector>

class Cvode;

class Daspk {
  public:
    Daspk(Cvode*, int neq);
    virtual ~Daspk();
    int init();
    int advance_tn(double tstop);
    int interpolate(double tout);  // has strict precondition
    void statistics();
    N_Vector ewtvec();
    N_Vector acorvec();

    // Three-panel IC audit (stdout or file). See dae_init_audit docs.
    static void audit_set_level(int level);
    static int audit_level();
    static void audit_arm_at(double t);
    static double audit_t_select();
    static void audit_set_file(const char* path);  // empty/null → stdout
    static const char* audit_file();

    // Last IC-time continuous Vector.play forcing t+ collection (A1; for A2 y').
    static const std::vector<NrnForcingTPlus>& last_forcing_tplus();
    static double last_forcing_t();

    // A5: cumulative IC path statistics (reset on CVode construction / clear).
    static void reset_ic_stats();
    static void print_ic_stats();  // used by statistics()
    // last IC: path_mode and forcing source bitmask (NRN_IC_FORCING_*)
    static int last_ic_path_mode();
    static int last_ic_forcing_flags();
    static int ic_mode3_ok_count();
    static int ic_mode3_fallback_count();

  private:
    void ida_init();
    void info();
    // Heuristic nano-step IC (legacy). Returns 0 on success / accepted warn path.
    int init_heuristic();
    // IDACalcIC(IDA_Y_INIT) with yp=0 + ODE f(y). Returns 0 on residual success.
    int init_ida_y_init();
    // Mode 3: battery content hold (LM / extracellular) + y' from C*y'=f(y).
    int init_battery();
    // Shared residual WRMS check and parasite / style handling after y,yp ready.
    int check_init_residual();

    // IC audit helpers
    bool audit_should_fire() const;
    // Save continuous (y, yp) and residual already in delta_ (caller just evaluated res).
    // Prefer calling after interpolate/retreat so panel A is at t_event, not step end tn.
    void audit_save_pre_from_delta();
    void audit_eval_residual(N_Vector y, N_Vector yp, double* max_abs, double* wrms);
    void audit_dump_panel(FILE* f,
                          const char* title,
                          N_Vector y,
                          N_Vector yp,
                          N_Vector delta,
                          double max_abs,
                          double wrms,
                          int max_rows);
    FILE* audit_open_out();

  public:
    void* mem_;
    Cvode* cv_;
    N_Vector yp_;
    N_Vector delta_;     // use for calling res explicitly
    N_Vector parasite_;  // used when initialization cannot make f(y',y,t)<tol
    double t_parasite_;
    bool use_parasite_;
    char* spmat_;
    static int init_failure_style_;
    static double dteps_;
    static int init_try_again_;
    static int first_try_init_failures_;
    // 0 = heuristic only (shipped default if NRN_DAE_INIT_MODE_DEFAULT unset);
    // 1 = IDA_Y_INIT then heuristic fallback;
    // 2 = IDA_Y_INIT only; 3 = battery content hold + y' from C*y'=f(y),
    //     with heuristic fallback on residual failure.
    static int init_mode_;
    // Count of mode 1/3 residual failures that fell back to the heuristic.
    static int calcic_fallback_count_;
    // A5: mode-3 specific + forcing-source tallies
    static int ic_init_count_;
    static int ic_mode3_ok_count_;
    static int ic_mode3_fallback_count_;
    static int ic_forcing_play_inits_;    // inits that used play b'
    static int ic_forcing_dforce_inits_;  // inits that used dforce/bdot
    static int ic_forcing_fd_inits_;      // inits that used FD of force callable
    static int last_ic_path_mode_;
    static int last_ic_forcing_flags_;

    // Audit control (process-wide; one IDA path typically).
    static int audit_level_;         // 0 off, 1 summary, 2 three-panel (top residual rows)
    static double audit_t_select_;   // first reinit with t >= this (when armed)
    static int audit_armed_;         // 1 = waiting for t match
    static int audit_serial_;        // reinit count (all reinits)
    static std::string audit_path_;  // empty → stdout

    // Forcing t+ from continuous Vector.play at last Daspk::init (A1).
    static std::vector<NrnForcingTPlus> last_forcing_tplus_;
    static double last_forcing_t_;

  private:
    // Continuous pre-reinit snapshot for panel A (play/NetCon/at_time retreat).
    // Residual is captured when res was evaluated with continuous play at that t
    // (do not re-eval after the discontinuity has been applied).
    std::vector<double> audit_y_pre_;
    std::vector<double> audit_yp_pre_;
    std::vector<double> audit_r_pre_;
    double audit_pre_t_;
    double audit_pre_max_abs_;
    double audit_pre_wrms_;
    int audit_pre_neq_;
    bool audit_pre_valid_;
    bool audit_pre_res_valid_;  // residual vector matches y/yp/t above
};

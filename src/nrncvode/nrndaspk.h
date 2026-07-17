#pragma once

#include "shared/nvector_serial.h"
#include "nvector_nrnthread.h"
#include "nvector_nrnthread_ld.h"
#include "nvector_nrnserial_ld.h"

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

  private:
    void ida_init();
    void info();
    // Heuristic nano-step IC (legacy). Returns 0 on success / accepted warn path.
    int init_heuristic();
    // IDACalcIC(IDA_Y_INIT) with yp=0 + ODE f(y). Returns 0 on residual success.
    int init_ida_y_init();
    // Spike: capacitors → voltage sources holding Δv; pure algebraic solve.
    int init_battery();
    // Shared residual WRMS check and parasite / style handling after y,yp ready.
    int check_init_residual();

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
    // 0 = heuristic only (default); 1 = IDA_Y_INIT then heuristic fallback;
    // 2 = IDA_Y_INIT only; 3 = battery IC (C→V hold) for LinearMechanism spike.
    static int init_mode_;
    // Count of times mode 1 fell back from IDA_Y_INIT to the heuristic.
    static int calcic_fallback_count_;
};

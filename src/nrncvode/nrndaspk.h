#ifndef daspk_h
#define daspk_h

#include <sundials/shared/nvector_serial.h>
#include <nrniv/nvector_nrnthread.h>
#include <nrniv/nvector_nrnthread_ld.h>
#include <nrniv/nvector_nrnserial_ld.h>

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
};
#endif

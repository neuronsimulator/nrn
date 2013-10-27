#include <nrnconf.h>
#include <nrnmpi.h>

extern "C" {
  extern void mk_mech();
  extern void mk_netcvode();
  extern void nrn_setup();
  extern void BBS_netpar_solve(double);
  extern void output_spikes();
}

int main(int argc, char** argv, char** env) {
  nrnmpi_init(1, &argc, &argv);
  mk_mech();
  mk_netcvode();
  nrn_setup();
  BBS_netpar_solve(100.);
  output_spikes();
  return 0;
}

#ifndef nrniv_dec_h
#define nrniv_dec_h

class PreSyn;

#if defined(__cplusplus)
extern "C" {
#endif

extern void mk_mech();
extern void mk_netcvode();
extern void nrn_setup();
extern void output_spikes();
extern void BBS_netpar_solve(double);
extern void nrn_cleanup_presyn(PreSyn*);
extern void nrn_outputevent(unsigned char, double);
extern void ncs2nrn_integrate(double tstop);
extern void nrn_pending_selfqueue(double, NrnThread*);

#if defined(__cplusplus)
}
#endif

#endif

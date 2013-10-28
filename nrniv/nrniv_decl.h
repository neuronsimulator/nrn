#ifndef nrniv_dec_h
#define nrniv_dec_h

#if defined(__cplusplus)
extern "C" {
#endif

extern void nrn_cleanup_presyn(PreSyn*);
extern void nrn_outputevent(unsigned char, double);
extern void ncs2nrn_integrate(double tstop);

#if defined(__cplusplus)
}
#endif

#endif

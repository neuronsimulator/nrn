#ifndef nrniv_dec_h
#define nrniv_dec_h

#include "src/simcore/nrniv/netcon.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern int nrn_mallinfo(void);
extern void mk_mech(const char* fname);
extern void mk_netcvode(void);
extern void nrn_setup(int ngroup, int* gidgroups, const char *path);
extern double BBS_netpar_mindelay(double maxdelay);
extern void BBS_netpar_solve(double);

extern void nrn_alloc_gid2out(int size, int poolsize);
extern void nrn_alloc_gid2in(int size, int poolsize);
extern int input_gid_register(int gid);
extern int input_gid_associate(int gid, InputPreSyn* psi);
extern void BBS_gid2ps(int gid, PreSyn** ps, InputPreSyn** psi);

extern NetCon* BBS_gid_connect(int gid, Point_process* target, NetCon&);
extern void BBS_cell(int gid, PreSyn* ps);
extern void BBS_set_gid2node(int gid, int rank);
extern void nrn_cleanup_presyn(DiscreteEvent*);
extern void nrn_outputevent(unsigned char, double);
extern void ncs2nrn_integrate(double tstop);
extern void nrn_pending_selfqueue(double, NrnThread*);
extern size_t output_presyn_size(int prnt);
extern size_t input_presyn_size(int prnt);

extern NetCon** netcon_in_presyn_order_;

#if defined(__cplusplus)
}
#endif

#endif

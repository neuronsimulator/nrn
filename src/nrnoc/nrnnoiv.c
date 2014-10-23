#include <../../nrnconf.h>
#include "nrnredef.h"
#include "section.h"
#include "nrnmpiuse.h"
#include "nrndae_c.h"
#define hoc_retpushx hoc_retpushx

/* stubs for nrnoc. The actual functions are for interviews menus */
void nrnallsectionmenu(){hoc_retpushx(0);}
void nrnallpointmenu(){hoc_retpushx(0);}
void nrnsecmenu(){hoc_retpushx(0);}
void nrnglobalmechmenu(){hoc_retpushx(0);}
void nrnmechmenu(){hoc_retpushx(0);}
void nrnpointmenu(){hoc_retpushx(0);}
void make_mechanism(){hoc_retpushx(0);}
void make_pointprocess(){hoc_retpushx(0);}
void nrnpython() {hoc_retpushx(0);}
/*ARGSUSED*/
void hoc_construct_point(Object* ob, int i){}
void nrn_random_play(){}
void* nrn_random_arg(i) int i; { return (void*)0; }
double nrn_random_pick(r) void* r; { return 0.; }
void hoc_new_opoint(){}
int special_pnt_call(Object* ob, Symbol* sym, int narg){return 0;}
void bbs_handle(){}

void nrndae_alloc(void){}
int nrndae_extra_eqn_count(void){return 0;}
void nrndae_init(void){}
void nrndae_rhs(void){}
void nrndae_lhs(void){}
void nrndae_dkmap(double** pv, double** pvdot){}
void nrndae_dkres(double* y, double* yprime, double* delta){}
void nrndae_dkpsol(double unused){}
void nrndae_update(void){}
int nrndae_list_is_empty(void){return 0;}

void nrn_solver_prepare(){}
void nrn_fihexec(i) int i;{}
void nrn_deliver_events(tt) double tt;{}
void nrn_record_init(){}
void nrn_play_init(){}
void fixed_record_continuous(){}
void fixed_play_continuous(){}
void nrniv_recalc_ptrs(){}
void nrn_recalc_ptrvector(){}
void nrn_extra_scatter_gather(int direction, int tid) {}
void nrn_update_ion_pointer(int type, Datum* d, int i, int j) {}
void nrn_update_ps2nt(){}

int at_time(NrnThread* nt, double te) {
	double x = te - 1e-11;
	if (x > (nt->_t - nt->_dt)  && x <= nt->_t) {
		return 1;
	}else{
		return 0;
	}
}

void net_event(){hoc_execerror("net_event only available in nrniv", (char*)0);}
void net_send(){hoc_execerror("net_send only available in nrniv", (char*)0);}
void artcell_net_send(){hoc_execerror("net_send only available in nrniv", (char*)0);}
void net_move(){hoc_execerror("net_move only available in nrniv", (char*)0);}
void artcell_net_move(){hoc_execerror("net_move only available in nrniv", (char*)0);}
void nrn_use_daspk(i) int i; { }

#if CVODE
void cvode_fadvance(t)double t;{}
void cvode_finitialize(double t0){}
void nrncvode_set_t(double tt) {}
void cvode_event(x) double x;{}
void clear_event_queue(){}
void init_net_events(){}
void deliver_net_events(){}
void hoc_reg_singlechan(){}
void _singlechan_declare(){}
void _nrn_single_react(){}
#endif

#if defined(CYGWIN)
void* dll_lookup(s) char* s; {return 0;}
void* dll_load(v, s) void* v; char* s; {return 0;}
#endif

void nrn_spike_exchange_init(){}
void nrn_spike_exchange(NrnThread* nt){}
void nrn_fake_fire(i,x,j)int i, j; double x;{}
void nrn_daq_ao() {}
void nrn_daq_ai() {}
void nrn_daq_scanstart(){}

void nrn_multisplit_ptr_update(){}
void nrn_multisplit_bksub(){assert(0);}
void nrn_multisplit_reduce_solve(){assert(0);}
void nrn_multisplit_triang(){assert(0);}
#if 1 || PARANEURON
double* nrn_classicalNodeA(Node* n) {return (double*)0;}
double* nrn_classicalNodeB(Node* n) {return (double*)0;}
#endif
void* nrn_pool_create(long count, int itemsize){assert(0); return (void*)0;}
void nrn_pool_delete(void* pool){assert(0);}
void nrn_pool_freeall(void* pool){assert(0);}
void* nrn_pool_alloc(void* pool){assert(0); return (void*)0;}

#if NRN_MUSIC
void nrnmusic_init(int* parg, char*** pargv){}      
void nrnmusic_terminate(){}      
#endif


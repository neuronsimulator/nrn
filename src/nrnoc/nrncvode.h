#ifndef nrncvode_h
#define nrncvode_h


extern "C" void cvode_fadvance(double);
extern void cvode_finitialize(double);
extern void nrncvode_set_t(double);
extern void deliver_net_events(NrnThread*);
extern void nrn_deliver_events(NrnThread*);
extern "C" void clear_event_queue(void);
extern void init_net_events(void);
extern void nrn_record_init(void);
extern void nrn_play_init(void);
extern void fixed_record_continuous(NrnThread* nt);
extern void fixed_play_continuous(NrnThread* nt);
extern void nrn_solver_prepare(void);
extern "C" void nrn_random_play();
extern void nrn_daspk_init_step(double, double, int);
extern void nrndae_init(void);
extern void nrndae_update(void);
extern void nrn_update_2d(NrnThread*);
extern void nrn_capacity_current(NrnThread* _nt, Memb_list* ml);
extern void nrn_spike_exchange_init(void);
extern void nrn_spike_exchange(NrnThread* nt);
extern bool nrn_use_bin_queue_;

#endif

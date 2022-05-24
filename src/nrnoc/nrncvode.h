#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void clear_event_queue(void);
void cvode_fadvance(double);
void nrn_random_play();
#ifdef __cplusplus
}
extern bool nrn_use_bin_queue_;
#endif
struct Memb_list;
struct NrnThread;
extern void cvode_finitialize(double);
extern void nrncvode_set_t(double);
extern void deliver_net_events(struct NrnThread*);
extern void nrn_deliver_events(struct NrnThread*);
extern void init_net_events(void);
extern void nrn_record_init(void);
extern void nrn_play_init(void);
extern void fixed_record_continuous(struct NrnThread* nt);
extern void fixed_play_continuous(struct NrnThread* nt);
extern void nrn_solver_prepare(void);
extern void nrn_daspk_init_step(double, double, int);
extern void nrndae_init(void);
extern void nrndae_update(void);
extern void nrn_update_2d(struct NrnThread*);
extern void nrn_capacity_current(struct NrnThread* _nt, struct Memb_list* ml);
extern void nrn_spike_exchange_init(void);
extern void nrn_spike_exchange(struct NrnThread* nt);

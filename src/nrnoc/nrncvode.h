#pragma once
#include "oc_ansi.h"  // neuron::model_sorted_token
struct Memb_list;
struct NrnThread;
void cvode_fadvance(double);
extern void cvode_finitialize(double);
extern void nrncvode_set_t(double);
extern void deliver_net_events(NrnThread*);
extern void nrn_deliver_events(NrnThread*);
void clear_event_queue();
void free_event_queues();
extern void init_net_events();
extern void nrn_record_init();
extern void nrn_play_init();
void fixed_record_continuous(neuron::model_sorted_token const&, NrnThread& nt);
extern void fixed_play_continuous(NrnThread* nt);
extern void nrn_solver_prepare();
extern "C" void nrn_random_play();
extern void nrn_daspk_init_step(double, double, int);
extern void nrndae_init();
extern void nrndae_update(NrnThread*);
extern void nrn_update_2d(NrnThread*);
void nrn_capacity_current(neuron::model_sorted_token const&, NrnThread* _nt, Memb_list* ml);
extern void nrn_spike_exchange_init();
void nrn_spike_exchange(NrnThread* nt);
extern bool nrn_use_bin_queue_;
double nrn_event_queue_stats(double* stats);
void nrn_fake_fire(int gid, double spiketime, int fake_out);

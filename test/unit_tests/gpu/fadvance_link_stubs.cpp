#include "multicore.h"
#include "neuron/model_data.hpp"
#include "nrn_ansi.h"
#include "nrncvode.h"

// Standalone fadvance tests link fadvance_gpu without libnrniv/fadvance.cpp.
void (*nrnthread_v_transfer_)(NrnThread*) = +[](NrnThread*) {};

extern "C" void nrn_random_play() {}

void deliver_net_events(NrnThread*) {}
void nrn_deliver_events(NrnThread*) {}
void fixed_play_continuous(NrnThread*) {}
// sync_after_vecplay calls this; without a definition the unit test binary
// resolves the call to 0x0 and SIGSEGVs.
bool nrn_thread_has_fixed_play(NrnThread*) {
    return false;
}
void setup_tree_matrix(neuron::model_sorted_token const&, NrnThread&) {}
void nrn_solve(NrnThread*) {}
void second_order_cur(NrnThread*) {}
void nrn_update_voltage(neuron::model_sorted_token const&, NrnThread&) {}
void nrn_fixed_step_lastpart(neuron::model_sorted_token const&, NrnThread&) {}

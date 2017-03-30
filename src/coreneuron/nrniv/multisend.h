#ifndef nrnmultisend_h
#define nrnmultisend_h

#include "coreneuron/nrnmpi/nrnmpiuse.h"

extern int use_multisend_;
extern int n_multisend_interval;
extern int use_phase2_;

class PreSyn;
class NrnThread;

void nrn_multisend_send(PreSyn*, double t, NrnThread*);
void nrn_multisend_receive(NrnThread*);  // must be thread 0
void nrn_multisend_advance();
void nrn_multisend_init();

void nrn_multisend_cleanup();
void nrn_multisend_setup();

void nrn_multisend_setup_targets(int use_phase2, int*& targets_phase1, int*& targets_phase2);

#endif  // nrnmultisend_h

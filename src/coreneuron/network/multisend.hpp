/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include "coreneuron/mpi/nrnmpiuse.h"
namespace coreneuron {
extern bool use_multisend_;
extern int n_multisend_interval;
extern bool use_phase2_;

class PreSyn;
struct NrnThread;

void nrn_multisend_send(PreSyn*, double t, NrnThread*);
void nrn_multisend_receive(NrnThread*);  // must be thread 0
void nrn_multisend_advance();
void nrn_multisend_init();

void nrn_multisend_cleanup();
void nrn_multisend_setup();

void nrn_multisend_setup_targets(bool use_phase2, int*& targets_phase1, int*& targets_phase2);
}  // namespace coreneuron

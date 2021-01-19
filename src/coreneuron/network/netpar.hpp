/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include "coreneuron/network/partrans.hpp"
#include "coreneuron/sim/multicore.hpp"

namespace coreneuron {

    extern void nrn_spike_exchange_init(void);
    extern void nrn_spike_exchange(NrnThread* nt);
}

/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include "coreneuron/sim/multicore.hpp"

namespace coreneuron {

/**
   If FOR_NETCON in use, setup NrnThread fornetcon related info.
**/

void setup_fornetcon_info(NrnThread& nt);

}  // namespace coreneuron

/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef _H_SETUP_FORNETCON_
#define _H_SETUP_FORNETCON_

#include "coreneuron/sim/multicore.hpp"

namespace coreneuron {

/**
   If FOR_NETCON in use, setup NrnThread fornetcon related info.
**/

void setup_fornetcon_info(NrnThread& nt);

}  // namespace coreneuron
#endif  //_H_SETUP_FORNETCON_

/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <vector>

#include "coreneuron/io/nrn_filehandler.hpp"
#include "coreneuron/utils/nrnmutdec.hpp"

namespace coreneuron {

struct NrnThread;

class Phase1 {
  public:
    Phase1(FileHandler& F);
    Phase1(int thread_id);
    void populate(NrnThread& nt, OMP_Mutex& mut);

  private:
    std::vector<int> output_gids;
    std::vector<int> netcon_srcgids;
    std::vector<int> netcon_negsrcgid_tid;  // entries only for negative srcgids
};

}  // namespace coreneuron

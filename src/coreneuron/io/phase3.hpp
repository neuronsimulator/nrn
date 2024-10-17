/*
# =============================================================================
# Copyright (c) 2016 - 2024 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include "coreneuron/io/nrn_filehandler.hpp"

namespace coreneuron {
struct NrnThreadMappingInfo;

class Phase3 {
  public:
    void read_file(FileHandler& F, NrnThreadMappingInfo* ntmapping);
    void read_direct(NrnThreadMappingInfo* ntmapping);
};
}  // namespace coreneuron

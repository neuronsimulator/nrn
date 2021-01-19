/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <string>

#include "coreneuron/network/netcon.hpp"
#include "coreneuron/network/netcvode.hpp"

namespace coreneuron {

#if defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
struct VarWithMapping {
    int id;
    double* var_value;
    VarWithMapping(int id_, double* v_)
        : id(id_)
        , var_value(v_) {}
};

// mapping the set of variables pointers to report to its gid
using VarsToReport = std::unordered_map<int, std::vector<VarWithMapping>>;

class ReportEvent: public DiscreteEvent {
  public:
    ReportEvent(double dt, double tstart, const VarsToReport& filtered_gids, const char* name);

    /** on deliver, call ReportingLib and setup next event */
    void deliver(double t, NetCvode* nc, NrnThread* nt) override;
    bool require_checkpoint() override;

  private:
    double dt;
    double step;
    std::string report_path;
    std::vector<int> gids_to_report;
    double tstart;
};
#endif  // defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)

}  // Namespace coreneuron

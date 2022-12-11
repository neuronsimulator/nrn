/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
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
    uint32_t id;
    double* var_value;
    VarWithMapping(int id_, double* v_)
        : id(id_)
        , var_value(v_) {}
};

// mapping the set of variables pointers to report to its gid
using VarsToReport = std::unordered_map<uint64_t, std::vector<VarWithMapping>>;

class ReportEvent: public DiscreteEvent {
  public:
    ReportEvent(double dt,
                double tstart,
                const VarsToReport& filtered_gids,
                const char* name,
                double report_dt);

    /** on deliver, call ReportingLib and setup next event */
    void deliver(double t, NetCvode* nc, NrnThread* nt) override;
    bool require_checkpoint() override;
    void summation_alu(NrnThread* nt);

  private:
    double dt;
    double step;
    std::string report_path;
    double report_dt;
    int reporting_period;
    std::vector<int> gids_to_report;
    double tstart;
    VarsToReport vars_to_report;
};
#endif  // defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)

}  // Namespace coreneuron

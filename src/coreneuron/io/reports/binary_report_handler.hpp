/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <array>

#include "report_handler.hpp"
#include "coreneuron/io/nrnsection_mapping.hpp"

namespace coreneuron {

class BinaryReportHandler: public ReportHandler {
  public:
    void create_report(ReportConfiguration& config, double dt, double tstop, double delay) override;
#ifdef ENABLE_BIN_REPORTS
    void register_section_report(const NrnThread& nt,
                                 const ReportConfiguration& config,
                                 const VarsToReport& vars_to_report,
                                 bool is_soma_target) override;
    void register_custom_report(const NrnThread& nt,
                                const ReportConfiguration& config,
                                const VarsToReport& vars_to_report) override;

  private:
    using create_extra_func = std::function<void(const CellMapping&, std::array<int, 5>&)>;
    void register_report(const NrnThread& nt,
                         const ReportConfiguration& config,
                         const VarsToReport& vars_to_report,
                         create_extra_func& create_extra);
#endif  // ENABLE_BIN_REPORTS
};

}  // Namespace coreneuron

/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <memory>
#include <vector>

#include "report_handler.hpp"

namespace coreneuron {

class SonataReportHandler: public ReportHandler {
  public:
    SonataReportHandler(ReportConfiguration& config)
        : ReportHandler(config) {}

    void create_report(double dt, double tstop, double delay) override;
#ifdef ENABLE_SONATA_REPORTS
    void register_soma_report(const NrnThread& nt,
                              ReportConfiguration& config,
                              const VarsToReport& vars_to_report) override;
    void register_compartment_report(const NrnThread& nt,
                                     ReportConfiguration& config,
                                     const VarsToReport& vars_to_report) override;
    void register_custom_report(const NrnThread& nt,
                                ReportConfiguration& config,
                                const VarsToReport& vars_to_report) override;

  private:
    void register_report(const NrnThread& nt,
                         ReportConfiguration& config,
                         const VarsToReport& vars_to_report);
#endif  // ENABLE_SONATA_REPORTS
};

}  // Namespace coreneuron

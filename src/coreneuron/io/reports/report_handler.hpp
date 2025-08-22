/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <memory>
#include <vector>

#include "nrnreport.hpp"
#include "coreneuron/io/reports/report_event.hpp"
#include "coreneuron/sim/multicore.hpp"

namespace coreneuron {

class ReportHandler {
  public:
    virtual ~ReportHandler() = default;

    virtual void create_report(ReportConfiguration& config, double dt, double tstop, double delay);
#ifdef ENABLE_SONATA_REPORTS
    virtual void register_section_report(const NrnThread& nt,
                                         const ReportConfiguration& config,
                                         const VarsToReport& vars_to_report,
                                         bool is_soma_target);
    virtual void register_custom_report(const NrnThread& nt,
                                        const ReportConfiguration& config,
                                        const VarsToReport& vars_to_report);
#endif
  protected:
#ifdef ENABLE_SONATA_REPORTS
    std::vector<std::unique_ptr<ReportEvent>> m_report_events;
#endif
};

}  // Namespace coreneuron

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

#include "report_handler.hpp"

namespace coreneuron {

class SonataReportHandler: public ReportHandler {
  public:
    SonataReportHandler(const SpikesInfo& spikes_info)
        : m_spikes_info(spikes_info) {}

    void create_report(ReportConfiguration& config, double dt, double tstop, double delay) override;
#ifdef ENABLE_SONATA_REPORTS
    void register_section_report(const NrnThread& nt,
                                 const ReportConfiguration& config,
                                 const VarsToReport& vars_to_report,
                                 bool is_soma_target) override;
    void register_custom_report(const NrnThread& nt,
                                const ReportConfiguration& config,
                                const VarsToReport& vars_to_report) override;

  private:
    void register_report(const NrnThread& nt,
                         const ReportConfiguration& config,
                         const VarsToReport& vars_to_report);
    std::pair<std::string, int> get_population_info(int gid);
#endif  // ENABLE_SONATA_REPORTS

  private:
    SpikesInfo m_spikes_info;
};

}  // Namespace coreneuron

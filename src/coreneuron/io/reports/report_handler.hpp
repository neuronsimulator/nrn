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
    ReportHandler(ReportConfiguration& config)
        : m_report_config(config){};
    virtual ~ReportHandler() = default;

    virtual void create_report(double dt, double tstop, double delay);
#if defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
    virtual void register_section_report(const NrnThread& nt,
                                         ReportConfiguration& config,
                                         const VarsToReport& vars_to_report,
                                         bool is_soma_target);
    virtual void register_custom_report(const NrnThread& nt,
                                        ReportConfiguration& config,
                                        const VarsToReport& vars_to_report);
    VarsToReport get_section_vars_to_report(const NrnThread& nt,
                                            const std::set<int>& target,
                                            double* report_variable,
                                            SectionType section_type,
                                            bool all_compartments) const;
    VarsToReport get_summation_vars_to_report(const NrnThread& nt,
                                              const std::set<int>& target,
                                              ReportConfiguration& report,
                                              const std::vector<int>& nodes_to_gids) const;
    VarsToReport get_synapse_vars_to_report(const NrnThread& nt,
                                            ReportConfiguration& report,
                                            const std::vector<int>& nodes_to_gids) const;
    std::vector<int> map_gids(const NrnThread& nt) const;
#endif  // defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
  protected:
    ReportConfiguration m_report_config;
#if defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
    std::vector<std::unique_ptr<ReportEvent>> m_report_events;
#endif  // defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
};

}  // Namespace coreneuron

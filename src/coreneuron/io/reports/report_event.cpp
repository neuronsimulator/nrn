/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "report_event.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/io/reports/nrnreport.hpp"
#include "coreneuron/utils/nrn_assert.h"
#ifdef ENABLE_BIN_REPORTS
#include "reportinglib/Records.h"
#endif  // ENABLE_BIN_REPORTS
#ifdef ENABLE_SONATA_REPORTS
#include "bbp/sonata/reports.h"
#endif  // ENABLE_SONATA_REPORTS

namespace coreneuron {

#if defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
ReportEvent::ReportEvent(double dt,
                         double tstart,
                         const VarsToReport& filtered_gids,
                         const char* name,
                         double report_dt)
    : dt(dt)
    , tstart(tstart)
    , report_path(name)
    , report_dt(report_dt)
    , vars_to_report(filtered_gids) {
    nrn_assert(filtered_gids.size());
    step = tstart / dt;
    reporting_period = static_cast<int>(report_dt / dt);
    gids_to_report.reserve(filtered_gids.size());
    for (const auto& gid: filtered_gids) {
        gids_to_report.push_back(gid.first);
    }
    std::sort(gids_to_report.begin(), gids_to_report.end());
}

void ReportEvent::summation_alu(NrnThread* nt) {
    // Sum currents only on reporting steps
    if (step > 0 && (static_cast<int>(step) % reporting_period) == 0) {
        auto& summation_report = nt->summation_report_handler_->summation_reports_[report_path];
        // Add currents of all variables in each segment
        double sum = 0.0;
        for (const auto& kv: summation_report.currents_) {
            int segment_id = kv.first;
            for (const auto& value: kv.second) {
                double current_value = *value.first;
                int scale = value.second;
                sum += current_value * scale;
            }
            summation_report.summation_[segment_id] = sum;
            sum = 0.0;
        }
        // Add all currents in the soma
        // Only when type summation and soma target
        if (!summation_report.gid_segments_.empty()) {
            double sum_soma = 0.0;
            for (const auto& kv: summation_report.gid_segments_) {
                int gid = kv.first;
                for (const auto& segment_id: kv.second) {
                    sum_soma += summation_report.summation_[segment_id];
                }
                *(vars_to_report[gid].front().var_value) = sum_soma;
                sum_soma = 0.0;
            }
        }
    }
}

/** on deliver, call ReportingLib and setup next event */
void ReportEvent::deliver(double t, NetCvode* nc, NrnThread* nt) {
/* reportinglib is not thread safe */
#pragma omp critical
    {
        summation_alu(nt);
        // each thread needs to know its own step
#ifdef ENABLE_BIN_REPORTS
        records_nrec(step, gids_to_report.size(), gids_to_report.data(), report_path.data());
#endif
#ifdef ENABLE_SONATA_REPORTS
        sonata_record_node_data(step,
                                gids_to_report.size(),
                                gids_to_report.data(),
                                report_path.data());
#endif
        send(t + dt, nc, nt);
        step++;
    }
}

bool ReportEvent::require_checkpoint() {
    return false;
}
#endif  // defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)

}  // Namespace coreneuron

/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "report_event.hpp"

#include <numeric>

#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/io/reports/nrnreport.hpp"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/io/nrnsection_mapping.hpp"
#ifdef ENABLE_SONATA_REPORTS
#include "bbp/sonata/reports.h"
#endif  // ENABLE_SONATA_REPORTS

namespace coreneuron {

#ifdef ENABLE_SONATA_REPORTS
ReportEvent::ReportEvent(double dt,
                         double tstart,
                         const VarsToReport& filtered_gids,
                         const char* name,
                         double report_dt,
                         ReportType type)
    : dt(dt)
    , tstart(tstart)
    , report_path(name)
    , report_dt(report_dt)
    , report_type(type)
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

// Compute and store the weighted sum of currents for each segment,
// then compute the soma total as the sum of its segments' currents.
// This function assumes that nt->summation_report_handler_ and vars_to_report are properly
// populated.

void ReportEvent::summation_alu(NrnThread* nt) {
    auto& summation_report = nt->summation_report_handler_->summation_reports_[report_path];
    const auto weighted_sum_accumulator = [](double acc, const auto& pair) {
        // pair.first is a pointer to the current value,
        // pair.second is the scaling factor
        return acc + (*pair.first) * pair.second;
    };

    // Calculate weighted sum of currents for each segment using std::accumulate
    for (const auto& [segment_id, current_pairs]: summation_report.currents_) {
        double sum = std::accumulate(current_pairs.begin(),
                                     current_pairs.end(),
                                     0.0,
                                     weighted_sum_accumulator);

        summation_report.summation_[segment_id] = sum;
    }

    // Calculate the soma total current by summing over its segments using std::accumulate
    if (!summation_report.gid_segments_.empty()) {
        const auto accumulator = [&summation_report](double acc, int segment_id) {
            return acc + summation_report.summation_[segment_id];
        };
        for (const auto& [gid, segment_ids]: summation_report.gid_segments_) {
            double sum_soma =
                std::accumulate(segment_ids.begin(), segment_ids.end(), 0.0, accumulator);

            *(vars_to_report[gid].front().var_value) = sum_soma;
        }
    }
}

/** @brief Compute Local Field Potentials (LFP) for each cell.
 *
 * For every segment of a cell, computes the total membrane current (imem + iclamp)
 * and accumulates the weighted contribution to each electrode using precomputed
 * transfer factors. Results are written into the report output buffers.
 */
void ReportEvent::lfp_calc(NrnThread* nt) {
    auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt->mapping);
    double* fast_imem_rhs = nt->nrn_fast_imem->nrn_sav_rhs;
    auto& summation_report = nt->summation_report_handler_->summation_reports_[report_path];
    for (const auto& kv: vars_to_report) {
        int gid = kv.first;
        const auto& electrode_outputs = kv.second;
        const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
        const auto n_electrodes = cell_mapping->num_electrodes();
        const auto n_segments = cell_mapping->lfp_segment_ids.size();
        std::vector<double> lfp_values(n_electrodes, 0.0);
        for (size_t i = 0; i < n_segments; i++) {
            const auto segment_id = cell_mapping->lfp_segment_ids[i];

            // compute imem + iclamp
            const double imem = std::accumulate(summation_report.currents_[segment_id].begin(),
                                                summation_report.currents_[segment_id].end(),
                                                fast_imem_rhs[segment_id],
                                                [](double sum, const auto& value) {
                                                    return sum + *value.first * value.second;
                                                });

            // dot product with the factors
            const double* factors = &cell_mapping->lfp_factors_flat[i * n_electrodes];
            for (size_t e = 0; e < n_electrodes; e++) {
                lfp_values[e] += imem * factors[e];
            }
        }

        // write LFP values to report output buffers
        for (const auto& output: electrode_outputs) {
            *(output.var_value) = lfp_values[output.id];
        }
    }
}

/** on deliver, call ReportingLib and setup next event */
void ReportEvent::deliver(double t, NetCvode* nc, NrnThread* nt) {
/* libsonata is not thread safe */
#pragma omp critical
    {
        // Sum currents and calculate lfp only on reporting steps
        if ((static_cast<int>(step) % reporting_period) == 0) {
            if (report_type == ReportType::Summation) {
                summation_alu(nt);
            } else if (report_type == ReportType::LFP) {
                lfp_calc(nt);
            }
        }
        // each thread needs to know its own step
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
#endif

}  // Namespace coreneuron

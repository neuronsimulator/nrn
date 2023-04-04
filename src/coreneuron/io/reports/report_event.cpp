/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "report_event.hpp"
#include "coreneuron/apps/corenrn_parameters.hpp"
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
                         const std::vector<int>& gids,
                         const VarsToReport& filtered_gids,
                         const char* name,
                         double report_dt,
                         ReportType type,
                         bool all_compartments)
    : dt(dt)
    , tstart(tstart)
    , report_path(name)
    , report_dt(report_dt)
    , report_type(type)
    , gids_to_report(gids)
    , report_all_compartments(all_compartments)
    , vars_to_report(filtered_gids) {
    nrn_assert(filtered_gids.size());
    step = tstart / dt;
    reporting_period = static_cast<int>(report_dt / dt);
    std::sort(gids_to_report.begin(), gids_to_report.end());
}

void ReportEvent::summation_alu(NrnThread* nt) {
    auto& summation_report = nt->summation_report_handler_->summation_reports_[report_path];
    // Add currents of all variables in each segment
    for (const auto& kv: summation_report.currents_) {
        double sum = 0.0;
        int segment_id = kv.first;
        for (const auto& value: kv.second) {
            double current_value = *value.first;
            int scale = value.second;
            sum += current_value * scale;
        }
        summation_report.summation_[segment_id] = sum;
    }
    // Add all currents in the soma
    // Only when type summation and soma target
    if (!summation_report.gid_segments_.empty()) {
        for (const auto& kv: summation_report.gid_segments_) {
            double sum_soma = 0.0;
            int gid = kv.first;
            for (const auto& segment_id: kv.second) {
                sum_soma += summation_report.summation_[segment_id];
            }
            *(vars_to_report[gid].front().var_value) = sum_soma;
        }
    }
}

void ReportEvent::lfp_calc(NrnThread* nt) {
    auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt->mapping);
    double* fast_imem_rhs = nt->nrn_fast_imem->nrn_sav_rhs;
    auto& summation_report = nt->summation_report_handler_->summation_reports_[report_path];
    std::vector<double> sum_lfp_values;

    for (const auto& gid: gids_to_report) {
        const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
        int num_electrodes = cell_mapping->num_electrodes();
        std::vector<double> lfp_values(num_electrodes, 0.0);

        for (const auto& segment_factors: cell_mapping->lfp_factors) {
            int segment_id = segment_factors.first;
            const auto& factors = segment_factors.second;
            int electrode_id = 0;

            for (const auto& factor: factors) {
                double iclamp = calculate_iclamp(summation_report.currents_[segment_id]);
                lfp_values[electrode_id] += (fast_imem_rhs[segment_id] + iclamp) * factor;
                electrode_id++;
            }
        }

        if(report_all_compartments) {
            update_report_values(gid, lfp_values);
        } else {
            sum_lfp_values_for_electrodes(num_electrodes, sum_lfp_values, lfp_values);
        }
    }

    if(!report_all_compartments) {
        std::vector<double> global_sum_lfp_values;
#if NRNMPI
        if (corenrn_param.mpi_enable) {
            global_sum_lfp_values.resize(sum_lfp_values.size());
            int mpi_sum{1};
            nrnmpi_dbl_allreduce_vec(sum_lfp_values.data(), global_sum_lfp_values.data(), sum_lfp_values.size(), mpi_sum);
        } else
#endif
        {
            std::swap(sum_lfp_values, global_sum_lfp_values);
        }
        update_report_values(vars_to_report.begin()->first, global_sum_lfp_values);
    }
}

double ReportEvent::calculate_iclamp(const std::vector<std::pair<double*, int>>& currents) {
    double iclamp = 0.0;
    for (const auto& value: currents) {
        double current_value = *value.first;
        int scale = value.second;
        iclamp += current_value * scale;
    }
    return iclamp;
}

void ReportEvent::update_report_values(int gid, const std::vector<double>& lfp_values) {
    auto& to_report = vars_to_report[gid];
    for (int i = 0; i < to_report.size(); i++) {
        *(to_report[i].var_value) = lfp_values[i];
    }
}

void ReportEvent::sum_lfp_values_for_electrodes(int num_electrodes, std::vector<double>& sum_lfp_values, const std::vector<double>& lfp_values) {
    if (sum_lfp_values.empty()) {
        sum_lfp_values.resize(num_electrodes, 0.0);
    }
    for (int i=0; i < num_electrodes; i++) {
        sum_lfp_values[i] += lfp_values[i];
    }
}

/** on deliver, call ReportingLib and setup next event */
void ReportEvent::deliver(double t, NetCvode* nc, NrnThread* nt) {
/* libsonata is not thread safe */
#pragma omp critical
    {
        // Sum currents and calculate lfp only on reporting steps
        if (step > 0 && (static_cast<int>(step) % reporting_period) == 0) {
            if (report_type == ReportType::SummationReport) {
                summation_alu(nt);
            } else if (report_type == ReportType::LFPReport) {
                lfp_calc(nt);
            }
        }
        // each thread needs to know its own step
        sonata_record_node_data(step,
                                gids_to_report.size(),
                                gids_to_report.data(),
                                report_path.data());
        send(t + dt, nc, nt);
        step++;
    }
}

bool ReportEvent::require_checkpoint() {
    return false;
}
#endif

}  // Namespace coreneuron

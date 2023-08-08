/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/io/lfp.hpp"
#include "coreneuron/io/nrnsection_mapping.hpp"
#include "coreneuron/io/reports/report_event.hpp"
#include "coreneuron/mpi/nrnmpi.h"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <iostream>

using namespace coreneuron;
using namespace coreneuron::lfputils;

template <typename F>
double integral(F f, double a, double b, int n) {
    double step = (b - a) / n;  // width of each small rectangle
    double area = 0.0;          // signed area
    for (int i = 0; i < n; i++) {
        area += f(a + (i + 0.5) * step) * step;  // sum up each small rectangle
    }
    return area;
}


TEST_CASE("LFP_PointSource_LineSource") {
#if NRNMPI
    nrnmpi_init(nullptr, nullptr, false);
#endif
    double segment_length{1.0e-6};
    double segment_start_val{1.0e-6};
    std::array<double, 3> segment_start = std::array<double, 3>{0.0, 0.0, segment_start_val};
    std::array<double, 3> segment_end =
        paxpy(segment_start, 1.0, std::array<double, 3>{0.0, 0.0, segment_length});
    double floor{1.0e-6};
    pi = 3.141592653589;

    std::array<double, 10> vals;
    double circling_radius{1.0e-6};
    std::array<double, 3> segment_middle{0.0, 0.0, 1.5e-6};
    double medium_resistivity_fac{1.0};
    for (auto k = 0; k < 10; k++) {
        std::array<double, 3> approaching_elec =
            paxpy(segment_middle, 1.0, std::array<double, 3>{0.0, 1.0e-5 - k * 1.0e-6, 0.0});
        std::array<double, 3> circling_elec =
            paxpy(segment_middle,
                  1.0,
                  std::array<double, 3>{0.0,
                                        circling_radius * std::cos(2.0 * pi * k / 10),
                                        circling_radius * std::sin(2.0 * pi * k / 10)});

        double analytic_approaching_lfp = line_source_lfp_factor(
            approaching_elec, segment_start, segment_end, floor, medium_resistivity_fac);
        double analytic_circling_lfp = line_source_lfp_factor(
            circling_elec, segment_start, segment_end, floor, medium_resistivity_fac);
        double numeric_circling_lfp = integral(
            [&](double x) {
                return 1.0 / std::max(floor,
                                      norm(paxpy(circling_elec,
                                                 -1.0,
                                                 paxpy(segment_end,
                                                       x,
                                                       paxpy(segment_start, -1.0, segment_end)))));
            },
            0.0,
            1.0,
            10000);
        // TEST of analytic vs numerical integration
        std::clog << "ANALYTIC line source " << analytic_circling_lfp
                  << " vs NUMERIC line source LFP " << numeric_circling_lfp << "\n";
        REQUIRE(Approx(analytic_circling_lfp).margin(1.0e-6) == numeric_circling_lfp);
        // TEST of LFP Flooring
        if (approaching_elec[1] < 0.866e-6) {
            REQUIRE(analytic_approaching_lfp == 1.0e6);
        }
        vals[k] = analytic_circling_lfp;
    }
    // TEST of SYMMETRY of LFP FORMULA
    for (size_t k = 0; k < 5; k++) {
        REQUIRE(std::abs((vals[k] - vals[k + 5]) /
                         std::max(std::abs(vals[k]), std::abs(vals[k + 5]))) < 1.0e-12);
    }
    std::vector<std::array<double, 3>> segments_starts = {{0., 0., 1.},
                                                          {0., 0., 0.5},
                                                          {0.0, 0.0, 0.0},
                                                          {0.0, 0.0, -0.5}};
    std::vector<std::array<double, 3>> segments_ends = {{0., 0., 0.},
                                                        {0., 0., 1.},
                                                        {0., 0., 0.5},
                                                        {0.0, 0.0, 0.0}};
    std::vector<double> radii{0.1, 0.1, 0.1, 0.1};
    std::vector<std::array<double, 3>> electrodes = {{0.0, 0.3, 0.0}, {0.0, 0.7, 0.8}};
    std::vector<int> indices = {0, 1, 2, 3};
    LFPCalculator<LineSource> lfp(segments_starts, segments_ends, radii, indices, electrodes, 1.0);
    lfp.template lfp<std::vector<double>>({0.0, 1.0, 2.0, 3.0});
    std::vector<double> res_line_source = lfp.lfp_values();
    LFPCalculator<PointSource> lfpp(
        segments_starts, segments_ends, radii, indices, electrodes, 1.0);
    lfpp.template lfp<std::vector<double>>({0.0, 1.0, 2.0, 3.0});
    std::vector<double> res_point_source = lfpp.lfp_values();
    REQUIRE(res_line_source[0] == Approx(res_point_source[0]).margin(1.0));
    REQUIRE(res_line_source[1] == Approx(res_point_source[1]).margin(1.0));
#if NRNMPI
    nrnmpi_finalize();
#endif
}

#ifdef ENABLE_SONATA_REPORTS
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

TEST_CASE("LFP_ReportEvent") {
    const std::string report_name = "compartment_report";
    const std::vector<uint64_t> gids = {42, 134};
    const std::vector<int> segment_ids = {0, 1, 2, 3, 4};
    std::vector<double> curr(segment_ids.size());

    NrnThread nt;
    nt.mapping = new NrnThreadMappingInfo;
    auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    // Generate mapinfo CellMapping
    for (const auto& gid: gids) {
        mapinfo->mappingvec.push_back(new CellMapping(gid));
        for (const auto& segment: segment_ids) {
            std::vector<double> lfp_factors{segment + 1.0, segment + 2.0};
            mapinfo->mappingvec.back()->add_segment_lfp_factor(segment, lfp_factors);
        }
    }
    mapinfo->prepare_lfp();
    // Total number of electrodes 2 gids * 2 factors

    CellMapping* c42 = mapinfo->mappingvec[0];
    CellMapping* c134 = mapinfo->mappingvec[1];
    REQUIRE(c42->lfp_factors.size() == 5);
    REQUIRE(c134->num_electrodes() == 2);

    // Pass _lfp variable to vars_to_report
    size_t offset_lfp = 0;
    VarsToReport vars_to_report;
    for (const auto& gid: gids) {
        std::vector<VarWithMapping> to_report;
        const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
        int num_electrodes = cell_mapping->num_electrodes();
        for (int electrode_id = 0; electrode_id < num_electrodes; electrode_id++) {
            to_report.emplace_back(VarWithMapping(electrode_id, mapinfo->_lfp.data() + offset_lfp));
            offset_lfp++;
        }
        if (!to_report.empty()) {
            vars_to_report[gid] = to_report;
        }
    }

    // Generate summation for IClamp
    nt.summation_report_handler_ = std::make_unique<SummationReportMapping>(
        SummationReportMapping());
    for (const auto& segment_id: segment_ids) {
        curr[segment_id] = (segment_id + 1) / 10.0;
        nt.summation_report_handler_->summation_reports_[report_name]
            .currents_[segment_id]
            .push_back(std::make_pair(curr.data() + segment_id, -1));
    }

    // Generate currents
    std::vector<double> currents = {0.2, 0.4, 0.6, 0.8, 1.0};
    nt.nrn_fast_imem = new NrnFastImem;
    nt.nrn_fast_imem->nrn_sav_rhs = currents.data();

    const double dt = 0.025;
    const double tstart = 0.0;
    const double report_dt = 0.1;
    ReportType report_type = CompartmentReport;

    ReportEvent event(dt, tstart, vars_to_report, report_name.data(), report_dt, report_type);
    event.lfp_calc(&nt);

    REQUIRE(mapinfo->_lfp[0] == Approx(5.5).margin(1.0));
    REQUIRE(mapinfo->_lfp[3] == Approx(7.0).margin(1.0));

    delete mapinfo;
    delete nt.nrn_fast_imem;
}
#endif

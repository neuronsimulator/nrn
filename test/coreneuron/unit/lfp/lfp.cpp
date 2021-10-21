#define BOOST_TEST_MODULE LFPTest
#define BOOST_TEST_MAIN

#include <iostream>

#include <boost/test/unit_test.hpp>

#include "coreneuron/io/lfp.hpp"
#include "coreneuron/mpi/nrnmpi.h"

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


BOOST_AUTO_TEST_CASE(LFP_PointSource_LineSource) {
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
        BOOST_REQUIRE_CLOSE(analytic_circling_lfp, numeric_circling_lfp, 1.0e-6);
        // TEST of LFP Flooring
        BOOST_REQUIRE((approaching_elec[1] < 0.866e-6) ? analytic_approaching_lfp == 1.0e6 : true);
        vals[k] = analytic_circling_lfp;
    }
    // TEST of SYMMETRY of LFP FORMULA
    for (size_t k = 0; k < 5; k++) {
        BOOST_REQUIRE(std::abs((vals[k] - vals[k + 5]) /
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
    BOOST_REQUIRE_CLOSE(res_line_source[0], res_point_source[0], 1.0);
    BOOST_REQUIRE_CLOSE(res_line_source[1], res_point_source[1], 1.0);
#if NRNMPI
    nrnmpi_finalize();
#endif
}

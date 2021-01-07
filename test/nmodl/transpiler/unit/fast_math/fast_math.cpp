/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_MAIN

#include "codegen/fast_math.hpp"
#include <cmath>

#include <catch/catch.hpp>

float check_over_span_float(float f1(float),
                            float f2(float),
                            const float low_limit,
                            const float span,
                            const size_t npoints) {
    float x = 0.0;
    float val = f1(x) - f2(x);
    float err = val * val;  // 0.0 is an edge case


    for (size_t i = 0; i < npoints; ++i) {
        x = low_limit + span * i / npoints;
        val = f1(x) - f2(x);
        err += val * val;
    }
    err /= npoints + 1;
    return err;
}

double check_over_span_double(double f1(double),
                              double f2(double),
                              const double low_limit,
                              const double span,
                              const size_t npoints) {
    double x = 0.0;
    double val = f1(x) - f2(x);
    double err = val * val;  // 0.0 is an edge case

    for (size_t i = 0; i < npoints; ++i) {
        x = low_limit + span * i / npoints;
        val = f1(x) - f2(x);
        err += val * val;
    }
    err /= npoints + 1;
    return err;
}

SCENARIO("Check fast_math") {
    constexpr double low_limit = -5.0;
    constexpr double span = 10.0;
    constexpr size_t npoints = 1000;
    constexpr double err_threshold = 1e-10;
    GIVEN("vexp (double)") {
        const double err = check_over_span_double(std::exp, vexp, low_limit, span, npoints);

        THEN("error inside threshold") {
            REQUIRE(err < err_threshold);
        }
    }
    GIVEN("vexp (float)") {
        const double err = check_over_span_float(std::exp, vexp, low_limit, span, npoints);

        THEN("error inside threshold") {
            REQUIRE(err < err_threshold);
        }
    }
    GIVEN("expm1 (double)") {
        const double err =
            check_over_span_double([](const double x) -> double { return std::exp(x) - 1; },
                                   vexpm1,
                                   low_limit,
                                   span,
                                   npoints);

        THEN("error inside threshold") {
            REQUIRE(err < err_threshold);
        }
    }
    GIVEN("expm1 (float)") {
        const double err =
            check_over_span_float([](const float x) -> float { return std::exp(x) - 1; },
                                  vexpm1,
                                  low_limit,
                                  span,
                                  npoints);

        THEN("error inside threshold") {
            REQUIRE(err < err_threshold);
        }
    }
    GIVEN("exprelr (double)") {
        const double err = check_over_span_double(
            [](const double x) -> double { return (1.0 + x == 1.0) ? 1.0 : x / (std::exp(x) - 1); },
            exprelr,
            low_limit,
            span,
            npoints);

        THEN("error inside threshold") {
            REQUIRE(err < err_threshold);
        }
    }
    GIVEN("exprelr (float)") {
        const float err = check_over_span_float(
            [](const float x) -> float { return (1.0 + x == 1.0) ? 1.0 : x / (std::exp(x) - 1); },
            exprelr,
            low_limit,
            span,
            npoints);

        THEN("error inside threshold") {
            REQUIRE(err < err_threshold);
        }
    }
}

#include <cmath>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "vecplay_tplus.h"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

static double fd_deriv(const std::vector<double>& y,
                       const std::vector<double>& t,
                       double tt,
                       int ubound,
                       double h = 1e-9) {
    double v0 = 0., v1 = 0.;
    REQUIRE(nrn_vecplay_continuous_tplus(
                (int) y.size(), y.data(), t.data(), tt, ubound, &v0, nullptr) == 0);
    REQUIRE(nrn_vecplay_continuous_tplus(
                (int) y.size(), y.data(), t.data(), tt + h, ubound, &v1, nullptr) == 0);
    return (v1 - v0) / h;
}

TEST_CASE("vecplay continuous t+ before first knot", "[vecplay][tplus]") {
    std::vector<double> y{1., 2., 3.};
    std::vector<double> t{0., 1., 2.};
    const int ub = 2;
    double v = 0., d = 0.;
    REQUIRE(nrn_vecplay_continuous_tplus(3, y.data(), t.data(), -1.0, ub, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(1.0, 1e-15));
    REQUIRE_THAT(d, WithinAbs(0.0, 1e-15));
    // At t0: value y0, classical right derivative = outgoing slope (1 here)
    REQUIRE(nrn_vecplay_continuous_tplus(3, y.data(), t.data(), 0.0, ub, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(1.0, 1e-15));
    REQUIRE_THAT(d, WithinAbs(1.0, 1e-15));
}

TEST_CASE("vecplay continuous t+ interior and kink uses outgoing slope", "[vecplay][tplus]") {
    // Piecewise linear: slope 1 on [0,1], slope 2 on [1,2]
    std::vector<double> y{0., 1., 3.};
    std::vector<double> t{0., 1., 2.};
    const int ub = 2;
    double v = 0., d = 0.;

    REQUIRE(nrn_vecplay_continuous_tplus(3, y.data(), t.data(), 0.5, ub, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(0.5, 1e-15));
    REQUIRE_THAT(d, WithinAbs(1.0, 1e-15));

    // At knot t=1: outgoing segment slope 2, value 1
    REQUIRE(nrn_vecplay_continuous_tplus(3, y.data(), t.data(), 1.0, ub, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(1.0, 1e-15));
    REQUIRE_THAT(d, WithinAbs(2.0, 1e-15));

    REQUIRE(nrn_vecplay_continuous_tplus(3, y.data(), t.data(), 1.5, ub, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(2.0, 1e-15));
    REQUIRE_THAT(d, WithinAbs(2.0, 1e-15));
}

TEST_CASE("vecplay continuous t+ extrapolates last two points past end", "[vecplay][tplus]") {
    std::vector<double> y{0., 1., 3.};  // last slope (3-1)/(2-1) = 2
    std::vector<double> t{0., 1., 2.};
    const int ub = 2;
    double v = 0., d = 0.;
    REQUIRE(nrn_vecplay_continuous_tplus(3, y.data(), t.data(), 2.0, ub, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(3.0, 1e-15));
    REQUIRE_THAT(d, WithinAbs(2.0, 1e-15));
    REQUIRE(nrn_vecplay_continuous_tplus(3, y.data(), t.data(), 3.0, ub, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(5.0, 1e-15));  // 3 + 2*(3-2)
    REQUIRE_THAT(d, WithinAbs(2.0, 1e-15));
}

TEST_CASE("vecplay continuous t+ flat last segment hold after end", "[vecplay][tplus]") {
    std::vector<double> y{0., 1., 1.};
    std::vector<double> t{0., 1., 2.};
    const int ub = 2;
    double v = 0., d = 0.;
    REQUIRE(nrn_vecplay_continuous_tplus(3, y.data(), t.data(), 5.0, ub, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(1.0, 1e-15));
    REQUIRE_THAT(d, WithinAbs(0.0, 1e-15));
}

TEST_CASE("vecplay continuous t+ single sample", "[vecplay][tplus]") {
    std::vector<double> y{7.};
    std::vector<double> t{0.};
    double v = 0., d = 0.;
    REQUIRE(nrn_vecplay_continuous_tplus(1, y.data(), t.data(), 10.0, 0, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(7.0, 1e-15));
    REQUIRE_THAT(d, WithinAbs(0.0, 1e-15));
}

TEST_CASE("vecplay continuous t+ coincident times average value zero deriv", "[vecplay][tplus]") {
    std::vector<double> y{0., 2., 4.};
    std::vector<double> t{0., 1., 1.};  // jump disc at t=1
    const int ub = 2;
    double v = 0., d = 0.;
    // On the coincident segment at/after t=1 with ubound=2: average, deriv 0
    REQUIRE(nrn_vecplay_continuous_tplus(3, y.data(), t.data(), 1.0, ub, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(3.0, 1e-15));  // (2+4)/2
    REQUIRE_THAT(d, WithinAbs(0.0, 1e-15));
}

TEST_CASE("vecplay continuous t+ matches finite difference of value", "[vecplay][tplus]") {
    // Ramp like iramp: 0 until 1, then 0.5 at 1, 1.0 at 2
    std::vector<double> y{0., 0., 0.5, 1.0, 0., 0.};
    std::vector<double> t{0., 1., 1., 2., 2., 5.};
    // After disc at index of second t=1, active ubound for ramp segment is 3 (t=2,y=1)
    // Full vector ubound = 5
    const int ub = 5;
    for (double tt: {0.5, 1.0, 1.5, 2.0, 3.0, 4.0}) {
        double v = 0., d = 0.;
        REQUIRE(nrn_vecplay_continuous_tplus((int) y.size(), y.data(), t.data(), tt, ub, &v, &d) ==
                0);
        // Right FD of value should match classical deriv where smooth; at kinks
        // FD is one-sided outgoing if h>0
        const double d_fd = fd_deriv(y, t, tt, ub);
        REQUIRE_THAT(d, WithinAbs(d_fd, 1e-5));
    }
}

TEST_CASE("vecplay continuous t+ iramp segment after jump at t=1", "[vecplay][tplus]") {
    // Values: hold 0, jump to 0.5 at t=1, ramp to 1 at t=2, jump to 0
    // t: 0, 1, 1, 2, 2, 5  y: 0, 0, 0.5, 1, 0, 0
    std::vector<double> y{0., 0., 0.5, 1.0, 0., 0.};
    std::vector<double> t{0., 1., 1., 2., 2., 5.};
    double v = 0., d = 0.;

    // During ramp: ubound typically at least the end of ramp knot (index 3)
    // With full ubound=5, at tt=1.5 we are on segment between t[2]=1,y=0.5 and t[3]=2,y=1
    REQUIRE(nrn_vecplay_continuous_tplus(6, y.data(), t.data(), 1.5, 5, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(0.75, 1e-15));
    REQUIRE_THAT(d, WithinAbs(0.5, 1e-15));

    // At t=1+ with full vector: after coincident pair, outgoing is ramp slope 0.5
    // search at tt=1: first t[j] > 1 → j=3 (t[3]=2), segment (2,3): 0.5→1, slope 0.5
    REQUIRE(nrn_vecplay_continuous_tplus(6, y.data(), t.data(), 1.0, 5, &v, &d) == 0);
    REQUIRE_THAT(v, WithinAbs(0.5, 1e-15));
    REQUIRE_THAT(d, WithinAbs(0.5, 1e-15));
}

TEST_CASE("vecplay continuous t+ bad args", "[vecplay][tplus]") {
    double v = 0., d = 0.;
    REQUIRE(nrn_vecplay_continuous_tplus(0, nullptr, nullptr, 0., 0, &v, &d) == -1);
}

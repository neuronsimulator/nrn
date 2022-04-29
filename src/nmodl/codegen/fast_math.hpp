/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *
 * Note: fast_math.ispc translated into .hpp syntax. More information in
 * fast_math.ispc
 *************************************************************************/

#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

/**
 * \file
 * \brief Implementation of different math functions
 */

namespace nmodl {
namespace fast_math {

static inline double uint642dp(uint64_t x) {
    static_assert(sizeof(double) == sizeof(uint64_t),
                  "nmodl::fast_math::uint642dp requires sizeof(double) == sizeof(uint64_t)");
    double v;
    std::memcpy(&v, &x, sizeof(double));
    return v;
}

static inline uint64_t dp2uint64(double x) {
    static_assert(sizeof(double) == sizeof(uint64_t),
                  "nmodl::fast_math::dp2uint64 requires sizeof(double) == sizeof(uint64_t)");
    uint64_t v;
    std::memcpy(&v, &x, sizeof(uint64_t));
    return v;
}

static inline float int322sp(int32_t x) {
    static_assert(sizeof(float) == sizeof(int32_t),
                  "nmodl::fast_math::int322sp requires sizeof(float) == sizeof(int32_t)");
    float v;
    std::memcpy(&v, &x, sizeof(float));
    return v;
}

static inline unsigned int sp2uint32(float x) {
    static_assert(sizeof(float) == sizeof(unsigned int),
                  "nmodl::fast_math::sp2uint32 requires sizeof(float) == sizeof(unsigned int)");
    unsigned int v;
    std::memcpy(&v, &x, sizeof(unsigned int));
    return v;
}

static inline float f_inf() {
    static_assert(sizeof(float) == sizeof(uint32_t),
                  "nmodl::fast_math::f_inf requires sizeof(float) == sizeof(uint32_t)");
    float v;
    uint32_t int_val{0x7F800000};
    std::memcpy(&v, &int_val, sizeof(float));
    return v;
}

static inline double inf() {
    static_assert(sizeof(double) == sizeof(uint64_t),
                  "nmodl::fast_math::inf requires sizeof(double) == sizeof(uint64_t)");
    double v;
    uint64_t int_val{0x7FF0000000000000};
    std::memcpy(&v, &int_val, sizeof(double));
    return v;
}


static constexpr double EXP_LIMIT = 708.0;

static constexpr double C1 = 6.93145751953125E-1;
static constexpr double C2 = 1.42860682030941723212E-6;

static constexpr double PX1exp = 1.26177193074810590878E-4;
static constexpr double PX2exp = 3.02994407707441961300E-2;
static constexpr double PX3exp = 9.99999999999999999910E-1;
static constexpr double QX1exp = 3.00198505138664455042E-6;
static constexpr double QX2exp = 2.52448340349684104192E-3;
static constexpr double QX3exp = 2.27265548208155028766E-1;
static constexpr double QX4exp = 2.00000000000000000009;

static constexpr double LOG2E = 1.4426950408889634073599;  // 1/ln(2)

static constexpr float MAXLOGF = 88.72283905206835f;
static constexpr float MINLOGF = -88.f;

static constexpr float C1F = 0.693359375f;
static constexpr float C2F = -2.12194440e-4f;

static constexpr float PX1expf = 1.9875691500E-4f;
static constexpr float PX2expf = 1.3981999507E-3f;
static constexpr float PX3expf = 8.3334519073E-3f;
static constexpr float PX4expf = 4.1665795894E-2f;
static constexpr float PX5expf = 1.6666665459E-1f;
static constexpr float PX6expf = 5.0000001201E-1f;

static constexpr float LOG2EF = 1.44269504088896341f;  // 1/ln(2)

static constexpr double LOG10E = 0.4342944819032518;  // 1/log(10)
static constexpr float LOG10F = 0.4342945f;           // 1/log(10)

static inline double egm1(double x, double n) {
    // this cannot be reordered for the double-double trick to work
    // i.e., it cannot be re-written as g = x - n * (C1+C2)
    // the loss of accuracy comes from the different magnitudes of ln(2) and n
    // max(|n|) ~ 2^9
    // ln(2) ~ 2^-1
    volatile double g = x - n * C1;
    g -= n * C2;

    const double gg = g * g;

    double px = PX1exp;
    px *= gg;
    px += PX2exp;
    px *= gg;
    px += PX3exp;
    px *= g;

    double qx = QX1exp;
    qx *= gg;
    qx += QX2exp;
    qx *= gg;
    qx += QX3exp;
    qx *= gg;
    qx += QX4exp;

    return 2.0 * px / (qx - px);
}

/// double precision exp function
static inline double vexp(double initial_x) {
    double x = initial_x;
    double px = std::floor(LOG2E * x + 0.5);
    const int32_t n = px;

    x = 1.0 + egm1(x, px);
    x *= uint642dp((((uint64_t) n) + 1023) << 52);

    if (initial_x > EXP_LIMIT)
        x = inf();
    if (initial_x < -EXP_LIMIT)
        x = 0.0;

    return x;
}

/// double precision exp(x) - 1 function, used for small x.
static inline double vexpm1(double initial_x) {
    double x = initial_x;
    double px = std::floor(LOG2E * x + 0.5);
    const int32_t n = px;

    const uint64_t twopnm1 = (static_cast<uint64_t>(n - 1) + 1023) << 52;
    x = 2 * (uint642dp(twopnm1) * egm1(x, px) + uint642dp(twopnm1)) - 1;

    if (initial_x > EXP_LIMIT)
        x = inf();
    if (initial_x < -EXP_LIMIT)
        x = -1.0;

    return x;
}


/// double precision exprelr function ()
static inline double exprelr(double initial_x) {
    if (1.0 + initial_x == 1.0) {
        return 1.0;
    }

    return initial_x / vexpm1(initial_x);
}

static inline float egm1(float x) {
    float z = x * PX1expf;
    z += PX2expf;
    z *= x;
    z += PX3expf;
    z *= x;
    z += PX4expf;
    z *= x;
    z += PX5expf;
    z *= x;
    z += PX6expf;
    z *= x * x;
    z += x;

    return z;
}

/// single precision exp function
static inline float vexp(float x) {
    float z = std::floor(LOG2EF * x + 0.5f);

    // this cannot be reordered for the double-double trick to work
    float volatile g = x - z * C1F;
    g -= z * C2F;
    const int32_t n = z;

    z = 1.0f + egm1(g);

    z *= int322sp((n + 0x7f) << 23);

    if (x > MAXLOGF)
        z = f_inf();
    if (x < MINLOGF)
        z = 0.f;

    return z;
}

/// single precision exp function
static inline float vexpm1(float x) {
    float z = std::floor(LOG2EF * x + 0.5f);

    // this cannot be reordered for the double-double trick to work
    volatile float g = x - z * C1F;
    g -= z * C2F;

    const int32_t n = z;

    const int32_t twopnm1 = ((n - 1) + 0x7f) << 23;
    float ret = 2 * (int322sp(twopnm1) * egm1(g) + int322sp(twopnm1)) - 1;

    if (x > MAXLOGF)
        ret = f_inf();
    if (x < MINLOGF)
        ret = -1.0f;

    return ret;
}

/// single precision exprelr function
static inline float exprelr(float x) {
    if (1.0 + x == 1.0) {
        return 1.0;
    }

    return x / vexpm1(x);
}

/// double precision log10 function
static inline double log10(double f) {
    return std::log(f) * LOG10E;
}

/// single precision log10 function
static inline float log10(float f) {
    return std::log(f) * LOG10F;
}

}  // namespace fast_math
}  // namespace nmodl

/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *
 * Note that the fast ISPC exponentials Based on VDT implementation of
 * D. Piparo et al. See https://github.com/dpiparo/vdt for additional
 * license information.
 *************************************************************************/

/**
 * \file
 * \brief Implementation of different math functions with ISPC
 */

static inline double uint642dp(unsigned int64 ll) {
    return *((varying double*) (&ll));
}

static inline unsigned int64 dp2uint64(double x) {
    return *((varying unsigned int64*) (&x));
}

static inline float uint322sp(int32 x) {
    return *((varying float*) (&x));
}

static inline unsigned int sp2uint32(float x) {
    return *((varying unsigned int32*) (&x));
}

static inline double dpfloor(const double x) {
    int32 ret = x;
    ret -= (sp2uint32(x) >> 31);
    return ret;
}

static inline float spfloor(const float x) {
    int32 ret = x;
    ret -= (sp2uint32(x) >> 31);
    return ret;
}

static inline uniform float f_inf() {
    uniform unsigned int32 v = 0x7F800000;
    return *((float*) (&v));
}

static inline uniform double inf() {
    uniform unsigned int64 v = 0x7FF0000000000000;
    return *((double*) (&v));
}


static const uniform double EXP_LIMIT = 708.d;

static const uniform double PX1exp = 1.26177193074810590878d-4;
static const uniform double PX2exp = 3.02994407707441961300d-2;
static const uniform double PX3exp = 9.99999999999999999910d-1;
static const uniform double QX1exp = 3.00198505138664455042d-6;
static const uniform double QX2exp = 2.52448340349684104192d-3;
static const uniform double QX3exp = 2.27265548208155028766d-1;
static const uniform double QX4exp = 2.00000000000000000009d;

static const uniform double LOG2E = 1.4426950408889634073599d;

static const uniform float MAXLOGF = 88.72283905206835f;
static const uniform float MINLOGF = -88.f;

static const uniform float C1F = 0.693359375f;
static const uniform float C2F = -2.12194440e-4f;

static const uniform float PX1expf = 1.9875691500E-4f;
static const uniform float PX2expf = 1.3981999507E-3f;
static const uniform float PX3expf = 8.3334519073E-3f;
static const uniform float PX4expf = 4.1665795894E-2f;
static const uniform float PX5expf = 1.6666665459E-1f;
static const uniform float PX6expf = 5.0000001201E-1f;

static const uniform float LOG2EF = 1.44269504088896341f;

/// double precision exp function
static inline double vexp(double initial_x) {
    double x = initial_x;
    double px = dpfloor(LOG2E * x + 0.5d);
    const int32 n = px;

    x -= px * 6.93145751953125d-1;
    x -= px * 1.42860682030941723212d-6;

    const double xx = x * x;

    px = PX1exp;
    px *= xx;
    px += PX2exp;
    px *= xx;
    px += PX3exp;
    px *= x;

    double qx = QX1exp;
    qx *= xx;
    qx += QX2exp;
    qx *= xx;
    qx += QX3exp;
    qx *= xx;
    qx += QX4exp;

    x = px / (qx - px);
    x = 1.0d + 2.0d * x;
    x *= uint642dp((((unsigned int64) n) + 1023) << 52);

    if (initial_x > EXP_LIMIT)
        x = inf();
    if (initial_x < -EXP_LIMIT)
        x = 0.d;

    return x;
}

/// single prevision exp function
static inline float vexp(float initial_x) {
    float x = initial_x;
    float z = spfloor(LOG2EF * x + 0.5f);

    x -= z * C1F;
    x -= z * C2F;
    const int32 n = z;
    const float x2 = x * x;

    z = x * PX1expf;
    z += PX2expf;
    z *= x;
    z += PX3expf;
    z *= x;
    z += PX4expf;
    z *= x;
    z += PX5expf;
    z *= x;
    z += PX6expf;
    z *= x2;
    z += x + 1.0f;

    z *= uint322sp((n + 0x7f) << 23);

    if (initial_x > MAXLOGF)
        z = f_inf();
    if (initial_x < MINLOGF)
        z = 0.f;

    return z;
}

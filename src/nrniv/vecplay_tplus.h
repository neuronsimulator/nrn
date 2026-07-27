#pragma once

/**
 * @file vecplay_tplus.h
 * @brief Continuous Vector.play right-limit value and classical derivative.
 *
 * **Forcing \(t^+\) info:** the right-limit value \(u(t^+)\) and classical
 * derivative \(u'(t^+)\) of an exogenous drive after a discontinuity (or at
 * finitialize). In the geometric / DAE literature this pair is the **1-jet**
 * of \(u\) at \(t^+\).
 *
 * Geometry matches `VecPlayContinuous::interpolate` for a given active upper
 * knot index `ubound_index` (use `n - 1` when the full `t` vector is active):
 * - \(t < t_0\): hold \(y_0\), derivative 0
 * - interior / at a knot (including \(t = t_0\)): linear segment; **outgoing** slope
 * - \(t \ge t_{\mathrm{ubound}}\): linear extrapolation of the last two points
 *   on that bound (constant last value only if that segment is flat, or if
 *   `ubound_index == 0`)
 * - coincident knot times \(t_i = t_{i+1}\): value average, derivative 0
 */

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

/** One continuous Vector.play forcing \(t^+\) sample (value + classical deriv). */
struct NrnForcingTPlus {
    double value{};
    double deriv{};
    int playrec_index{-1};
    int ubound_index{-1};
    char label[128]{};  // Vector object name if available
};

/**
 * Collect forcing \(t^+\) info from every VecPlayContinuous in the playrec list.
 * Call at IDA IC time with post-event continuous play state (ubound_index_ current).
 * Clears and fills `out`. Returns (int)out.size().
 */
int nrn_collect_forcing_tplus(double tt, std::vector<NrnForcingTPlus>& out);

/** Print collected forcing \(t^+\) lines to f (audit / debug). */
void nrn_dump_forcing_tplus(FILE* f, double tt, const std::vector<NrnForcingTPlus>& entries);

/**
 * Continuous play value and classical derivative at time `tt`.
 *
 * @param n             number of samples (length of y and t)
 * @param y             sample values
 * @param t             sample times (nondecreasing)
 * @param tt            query time
 * @param ubound_index  active upper knot (same role as VecPlayContinuous::ubound_index_)
 * @param value         output u(tt); may be null
 * @param deriv         output u'(tt) classical; may be null
 * @return 0 on success, -1 if n < 1 or y/t null
 */
inline int nrn_vecplay_continuous_tplus(int n,
                                        const double* y,
                                        const double* t,
                                        double tt,
                                        int ubound_index,
                                        double* value,
                                        double* deriv) {
    if (n < 1 || !y || !t) {
        return -1;
    }
    if (ubound_index < 0) {
        ubound_index = 0;
    }
    if (ubound_index >= n) {
        ubound_index = n - 1;
    }

    auto set = [&](double v, double d) {
        if (value) {
            *value = v;
        }
        if (deriv) {
            *deriv = d;
        }
    };

    // Match VecPlayContinuous::interpolate when tt >= t[ubound]
    if (tt >= t[ubound_index]) {
        if (ubound_index == 0) {
            set(y[0], 0.);
            return 0;
        }
        const double t0 = t[ubound_index - 1];
        const double t1 = t[ubound_index];
        const double x0 = y[ubound_index - 1];
        const double x1 = y[ubound_index];
        if (t0 == t1) {
            set(0.5 * (x0 + x1), 0.);
            return 0;
        }
        const double slope = (x1 - x0) / (t1 - t0);
        // linear extrapolation when tt > t1
        set(x0 + slope * (tt - t0), slope);
        return 0;
    }

    // Strictly before the first knot: hold (right-limit at t0 uses outgoing segment).
    if (tt < t[0]) {
        set(y[0], 0.);
        return 0;
    }

    // Find last like VecPlayContinuous::search: first index with t[last] > tt
    // (unique times ⇒ outgoing segment at a knot, including tt == t[0]).
    int last = 1;
    while (last < ubound_index && tt >= t[last]) {
        ++last;
    }
    while (last > 0 && tt < t[last - 1]) {
        --last;
    }
    if (last < 1) {
        last = 1;
    }
    if (last > ubound_index) {
        last = ubound_index;
    }

    const double t0 = t[last - 1];
    const double t1 = t[last];
    const double x0 = y[last - 1];
    const double x1 = y[last];
    if (t0 == t1) {
        set(0.5 * (x0 + x1), 0.);
        return 0;
    }
    const double slope = (x1 - x0) / (t1 - t0);
    set(x0 + slope * (tt - t0), slope);
    return 0;
}

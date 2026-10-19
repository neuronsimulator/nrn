#include <../../nrnconf.h>
/*
 * Copyright (C) 2018-2026 Michael Hines and NEURON contributors
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Algorithms for Fourier Transform Spectral Methods */

#define _USE_MATH_DEFINES

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <utility>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// to print diagnostic statements
// #define DEBUG_SPCTRM
// otherwise
#undef DEBUG_SPCTRM

#undef myfabs
#define myfabs fabs

#include "oc_ansi.h"

// Apple LLVM version 5.1 (clang-503.0.38) generates "unsequenced modification warning".
// #define SQUARE(a) ((x_=(a)) == 0.0 ? 0.0 : x_*x_)
static inline double SQUARE(double a) {
    return a * a;
}

/*
 * Self-contained BSD-3-clause Cooley-Tukey radix-2 real FFT and half-complex inverse FFT.
 *
 * Frequency domain format:
 *   data[0]   = Re(X_0) (DC)
 *   data[k]   = Re(X_k)
 *   data[n-k] = Im(X_k)  for k = 1 .. n/2 - 1
 *   data[n/2] = Re(X_{n/2}) (Nyquist)
 */

static inline void bit_reverse_permute(double* data, size_t n) {
    size_t j = 0;
    for (size_t i = 0; i < n - 1; ++i) {
        size_t k = n / 2;
        if (i < j) {
            std::swap(data[i], data[j]);
        }
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }
}

static inline int compute_logn(size_t n) {
    if (n == 0 || (n & (n - 1)) != 0) {
        return -1;
    }
    int logn = 0;
    while ((size_t(1) << logn) < n) {
        logn++;
    }
    return logn;
}

static void real_radix2_forward(double* data, size_t n, int logn) {
    bit_reverse_permute(data, n);

    size_t p = 1;
    size_t q = n;

    for (int i = 1; i <= logn; ++i) {
        size_t p_half = p;
        p *= 2;
        q /= 2;

        // Butterfly for a = 0
        for (size_t b = 0; b < q; ++b) {
            double t0 = data[b * p] + data[b * p + p_half];
            double t1 = data[b * p] - data[b * p + p_half];
            data[b * p] = t0;
            data[b * p + p_half] = t1;
        }

        // Butterflies for a = 1 .. p_half/2 - 1
        {
            double w_real = 1.0;
            double w_imag = 0.0;
            const double theta = -2.0 * M_PI / static_cast<double>(p);
            const double s = std::sin(theta);
            const double t = std::sin(theta * 0.5);
            const double s2 = 2.0 * t * t;

            for (size_t a = 1; a < p_half / 2; ++a) {
                const double tmp_real = w_real - s * w_imag - s2 * w_real;
                const double tmp_imag = w_imag + s * w_real - s2 * w_imag;
                w_real = tmp_real;
                w_imag = tmp_imag;

                for (size_t b = 0; b < q; ++b) {
                    double z0_real = data[b * p + a];
                    double z0_imag = data[b * p + p_half - a];
                    double z1_real = data[b * p + p_half + a];
                    double z1_imag = data[b * p + p - a];

                    double t0_real = z0_real + w_real * z1_real - w_imag * z1_imag;
                    double t0_imag = z0_imag + w_real * z1_imag + w_imag * z1_real;

                    double t1_real = z0_real - w_real * z1_real + w_imag * z1_imag;
                    double t1_imag = z0_imag - w_real * z1_imag - w_imag * z1_real;

                    data[b * p + a] = t0_real;
                    data[b * p + p - a] = t0_imag;
                    data[b * p + p_half - a] = t1_real;
                    data[b * p + p_half + a] = -t1_imag;
                }
            }
        }

        if (p_half > 1) {
            for (size_t b = 0; b < q; ++b) {
                data[b * p + p - p_half / 2] *= -1.0;
            }
        }
    }
}

static void halfcomplex_radix2_inverse(double* data, size_t n, int logn) {
    size_t p = n;
    size_t q = 1;
    size_t p_half = n / 2;

    for (int i = 1; i <= logn; ++i) {
        // Butterfly for a = 0
        for (size_t b = 0; b < q; ++b) {
            const double z0 = data[b * p];
            const double z1 = data[b * p + p_half];
            data[b * p] = z0 + z1;
            data[b * p + p_half] = z0 - z1;
        }

        // Butterflies for a = 1 .. p_half/2 - 1
        {
            double w_real = 1.0;
            double w_imag = 0.0;
            const double theta = 2.0 * M_PI / static_cast<double>(p);
            const double s = std::sin(theta);
            const double t = std::sin(theta * 0.5);
            const double s2 = 2.0 * t * t;

            for (size_t a = 1; a < p_half / 2; ++a) {
                const double tmp_real = w_real - s * w_imag - s2 * w_real;
                const double tmp_imag = w_imag + s * w_real - s2 * w_imag;
                w_real = tmp_real;
                w_imag = tmp_imag;

                for (size_t b = 0; b < q; ++b) {
                    double z0_real = data[b * p + a];
                    double z0_imag = data[b * p + p - a];
                    double z1_real = data[b * p + p_half - a];
                    double z1_imag = -data[b * p + p_half + a];

                    double t0_real = z0_real + z1_real;
                    double t0_imag = z0_imag + z1_imag;

                    double t1_real = z0_real - z1_real;
                    double t1_imag = z0_imag - z1_imag;

                    data[b * p + a] = t0_real;
                    data[b * p + p_half - a] = t0_imag;
                    data[b * p + p_half + a] = (w_real * t1_real - w_imag * t1_imag);
                    data[b * p + p - a] = (w_real * t1_imag + w_imag * t1_real);
                }
            }
        }

        if (p_half > 1) {
            for (size_t b = 0; b < q; ++b) {
                data[b * p + p_half / 2] *= 2.0;
                data[b * p + p_half + p_half / 2] *= -2.0;
            }
        }

        p_half /= 2;
        p /= 2;
        q *= 2;
    }

    bit_reverse_permute(data, n);

    const double norm = 1.0 / static_cast<double>(n);
    for (size_t i = 0; i < n; ++i) {
        data[i] *= norm;
    }
}

void nrngsl_realft(double* data, unsigned long n, int direction) {
    if (n <= 1) {
        return;
    }
    int logn = compute_logn(n);
    if (logn == -1) {
        hoc_execerror("n is not a power of 2", nullptr);
    }
    if (direction == 1) {
        real_radix2_forward(data, n, logn);
    } else {
        halfcomplex_radix2_inverse(data, n, logn);
    }
}

/*
  convlv()  -- convolution of a read data set with a response function
  the respns function must be stored in wrap-around order
*/

/* frequency domain format copy from gsl fft to NRC fft */
void nrn_gsl2nrc(double* x, double* y, unsigned long n) {
    unsigned long i, n2;
    n2 = n / 2;
    y[0] = x[0];
    if (n < 2) {
        return;
    }
    y[1] = x[n2];
    for (i = 1; i < n2; ++i) {
        y[2 * i] = x[i];
        y[2 * i + 1] = -x[n - i];
    }
}
void nrn_nrc2gsl(double* x, double* y, unsigned long n) {
    double xn, xn2;
    unsigned long i, n2;
    n2 = n / 2;
    xn = n;
    xn2 = .5 * xn;
    y[0] = x[0] * xn2;
    if (n < 2) {
        return;
    }
    y[n2] = x[1] * xn2;
    for (i = 1; i < n2; ++i) {
        y[i] = x[2 * i] * xn2;
        y[n - i] = -x[2 * i + 1] * xn2;
    }
}

void nrn_convlv(double* data,
                unsigned long n,
                double* respns,
                unsigned long m,
                int isign,
                double* ans) {
    // data and respns are modified.
    unsigned long i;
    double scl, x_;

    int n2 = n / 2;
    for (i = 1; i <= (m - 1) / 2; i++) {
        respns[n - i] = respns[m - i];
    }
    for (i = (m + 1) / 2; i < n - (m - 1) / 2; i++) {
        respns[i] = 0.0;
    }
    nrngsl_realft(data, n, 1);
    nrngsl_realft(respns, n, 1);
    ans[0] = data[0] * respns[0];
    for (i = 1; i < n2; ++i) {
        if (isign == 1) {
            ans[i] = data[i] * respns[i] - data[n - i] * respns[n - i];
            ans[n - i] = data[i] * respns[n - i] + data[n - i] * respns[i];
        } else if (isign == -1) {
            if ((scl = SQUARE(ans[i - 1]) + SQUARE(ans[i])) == 0.0) {
                hoc_execerror("Deconvolving at response zero in nrn_convlv", 0);
            }
            scl *= 2.0;
            ans[i] = (data[i] * respns[i] + data[n - i] * respns[n - i]) / scl;
            ans[i] = (data[i] * respns[n - i] - data[n - i] * respns[i]) / scl;
        } else {
            hoc_execerror("No meaning for isign in nrn_convlv", 0);
        }
    }
    ans[n2] = data[n2] * respns[n2];
    nrngsl_realft(ans, n, -1);
}


/*
  correl()  -- correlation of two real data sets
  see http://www.cv.nrao.edu/course/astr534/FourierTransforms.html SF16
*/
void nrn_correl(double* x, double* y, unsigned long n, double* z) {
    nrngsl_realft(x, n, 1);
    nrngsl_realft(y, n, 1);
    z[0] = x[0] * y[0];
    int n2 = n / 2;
    for (int i = 1; i < n2; ++i) {
        z[i] = x[i] * y[i] + x[n - i] * y[n - i];
        // with following sign, produces same result as old NRC code
        z[n - i] = y[i] * x[n - i] - y[n - i] * x[i];
    }
    z[n2] = x[n2] * y[n2];
    nrngsl_realft(z, n, -1);
}


/*
  spctrm()  -- power spectrum using fourier transforms
  modified to take array rather than data stream.
*/

#define WINDOW(j, a, b) (1.0 - myfabs((((j) -1) - (a)) * (b))) /* Bartlett */

// void nrn_spctrm(double* data, double* p, int m, int k)
void nrn_spctrm(double* data, double* psd, int setsize, int numsegpairs) {
    int j, k, cx, n;
    double a, ainv, wfac, x_;
    double* fftv;

    // 0 is index of first meaningful element of power spectral density
    for (j = 0; j <= setsize - 1; j++)
        psd[j] = 0.0;  // zero out any prior result

    // calc factor used to correct for window's effect on psd
    a = setsize;
    ainv = 1.0 / a;

    n = 2 * setsize;
    wfac = 0.0;
    for (j = 1; j <= n; j++)
        wfac += SQUARE(WINDOW(j, a, 1 / a));

#ifdef DEBUG_SPCTRM
    printf("setsize %d, numsegpairs %d\n", setsize, numsegpairs);
    printf("=============\n");
    printf("window values\n");
    for (j = 1; j <= n; j++)
        printf("%d  %f\n", j, WINDOW(j, a, 1 / a));
    printf("initial wfac %f\n", wfac);
#endif

    // storage for a data segment
    fftv = (double*) malloc((size_t) (n * sizeof(double)));

    cx = 0;  // data index
    for (k = 1; k <= numsegpairs * 2; k++) {
#ifdef DEBUG_SPCTRM
        printf("==============\n");
        printf("segment %d\n", k);
#endif

        // fill fftv with a segment of data
        for (j = 0; j < n; j++)
            fftv[j] = data[cx++];
#ifdef DEBUG_SPCTRM
        for (j = 0; j < n; j++)
            printf("datum %d  %f\n", j, fftv[j]);
        printf("--------------\n");
#endif

        cx -= setsize;  // next segment overlaps by this much

        // apply window
        for (j = 1; j <= n; j++)
            fftv[j - 1] *= WINDOW(j, a, 1 / a);

        // apply fft
        nrngsl_realft(fftv, n, 1);
#ifdef DEBUG_SPCTRM
        printf("\nfftv:\n");
        for (j = 0; j < n; j++)
            printf("%f  ", fftv[j]);
        printf("\n");
#endif

        // add to psd
        psd[0] += SQUARE(fftv[0]);
        for (j = 1; j < setsize; j++) {
            psd[j] += (SQUARE(fftv[j]) + SQUARE(fftv[n - j]));
        }
    }

    wfac = 1 / (wfac * n * numsegpairs);
    for (j = 0; j < setsize; j++)
        psd[j] *= wfac;
    psd[0] *= 0.5;

#ifdef DEBUG_SPCTRM
    printf("1/wfac for all but DC = %f\n", 1. / wfac);
    printf("1/wfac for DC = %f\n", 2. / wfac);
    printf("final results--\n");
    for (j = 0; j < setsize; j++)
        printf("%d  %f\n", j, psd[j]);
    printf("\ndone\n");
#endif

    free(fftv);
}
#undef WINDOW

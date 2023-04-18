#include <../../nrnconf.h>
#include "mcran4.h"
#include "oc_ansi.h"
#include "scoplib.h"

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

#undef small

typedef long int integer;
typedef float real;
typedef double doublereal;
typedef long int logical;
#define TRUE_  (1)
#define FALSE_ (0)

#define dmax(arg1, arg2) ((arg1 > arg2) ? arg1 : arg2)
#define dmin(arg1, arg2) ((arg1 < arg2) ? arg1 : arg2)

static int minfit_(integer* m,
                   integer* n,
                   doublereal* machep,
                   doublereal* tol,
                   doublereal* ab,
                   doublereal* q,
                   doublereal* e);
static int min_(integer* n,
                integer* j,
                integer* nits,
                doublereal* d2,
                doublereal* x1,
                doublereal* f1,
                logical* fk,
                doublereal (*f)(doublereal*, integer),
                doublereal* x,
                doublereal* t,
                doublereal* machep,
                doublereal* h);
static int sort_(integer* m, integer* n, doublereal* d, doublereal* v);
static int quad_(integer* n,
                 doublereal (*f)(doublereal*, integer),
                 doublereal* x,
                 doublereal* t,
                 doublereal* machep,
                 doublereal* h);
static int vcprnt_(integer* option, doublereal* v, integer* n);
static int print_(integer* n, doublereal* x, integer* prin, doublereal* fmin);
static int maprnt_(integer* option, doublereal* v, integer* m, integer* n);
static doublereal random_(integer* naught);
static doublereal flin_(integer* n,
                        integer* j,
                        doublereal* l,
                        doublereal (*f)(doublereal*, integer),
                        doublereal* x,
                        integer* nf,
                        doublereal* t);

/* Common Block Declarations */

struct GlobalStruct {
    doublereal fx, ldt, dmin_;
    integer nf, nl;
    doublereal *minfit_e, *flin_t;
} * global_;
static struct GlobalStruct* gstruct_alloc(int sz) {
    struct GlobalStruct* gs = (struct GlobalStruct*) hoc_Ecalloc(1, sizeof(struct GlobalStruct));
    gs->minfit_e = (doublereal*) hoc_Ecalloc(sz, sizeof(doublereal));
    gs->flin_t = (doublereal*) hoc_Ecalloc(sz, sizeof(doublereal));
    return gs;
}
static void gstruct_free(struct GlobalStruct* gs) {
    if (gs) {
        free(gs->minfit_e);
        free(gs->flin_t);
        free(gs);
    }
}

#define global_1 (*global_)

struct QStruct {
    doublereal *v, *q0, *q1, qa, qb, qc, qd0, qd1, qf1;
    integer size;
} * q_, *q_hoc;
static struct QStruct* qstruct_alloc(int sz) {
    struct QStruct* qs = (struct QStruct*) hoc_Ecalloc(1, sizeof(struct QStruct));
    qs->size = sz;
    qs->v = (doublereal*) hoc_Ecalloc(sz * sz, sizeof(doublereal));
    qs->q0 = (doublereal*) hoc_Ecalloc(sz, sizeof(doublereal));
    qs->q1 = (doublereal*) hoc_Ecalloc(sz, sizeof(doublereal));
    return qs;
}
static void qstruct_free(struct QStruct* qs) {
    if (qs) {
        free(qs->v);
        free(qs->q0);
        free(qs->q1);
        free(qs);
    }
}

static doublereal *d_, *d_hoc;

#define d   d_
#define q_1 (*q_)

static int praxstep = 0; /* if n returns after n iterations of main loop */

double praxis_pval(int i) {
    if (d_hoc) {
        if (i >= q_hoc->size || i < 0) {
            hoc_execerror("arg out of range", (char*) 0);
        }
        return d_hoc[i];
    } else {
        hoc_execerror("fit_praxis hasn't been called", (char*) 0);
    }
    return 0.;
}
double* praxis_paxis(int i) {
    if (q_hoc) {
        if (i >= q_hoc->size || i < 0) {
            hoc_execerror("arg out of range", (char*) 0);
        }
        return q_hoc->v + q_hoc->size * i;
    } else {
        hoc_execerror("fit_praxis hasn't been called", (char*) 0);
    }
    return (double*) 0;
}

int praxis_stop(int i) {
    int j = praxstep;
    praxstep = i;
    return j;
}

/* Table of constant values */

static integer c__1 = 1;
static integer c__2 = 2;
static integer c__0 = 0;
static integer c__10 = 10;
static integer c__4 = 4;
static integer c__3 = 3;

doublereal praxis(doublereal* t0,
                  doublereal* machep,
                  doublereal* h0,
                  integer nval,
                  integer* prin,
                  doublereal* x,
                  doublereal (*f)(doublereal*, integer),
                  doublereal* fmin,
                  char* after_quad) {
    integer *n, nval1;
    /* System generated locals */
    integer i__1, i__2, i__3;
    doublereal ret_val, d__1;

    /* Local variables */
    doublereal scbd;
    integer idim;
    logical illc;
    integer klmk;
    doublereal h, ldfac;
    integer i, j, k;
    doublereal s, t, *y, large, *z, small, value, f1;
    integer sz, sz1;
    integer k2;
    doublereal m2, m4, t2, df, dn;
    integer kl, ii;
    doublereal sf;
    integer kt;
    doublereal sl, vlarge;
    doublereal vsmall;
    integer km1, im1;
    doublereal dni, lds;
    integer ktm;

    /* for re-entrancy*/
    struct GlobalStruct* gsav;
    struct QStruct* qsav;
    double* dsav;

    sz = nval;
    sz1 = sz + 1;
    y = (doublereal*) hoc_Ecalloc(sz, sizeof(doublereal));
    z = (doublereal*) hoc_Ecalloc(sz, sizeof(doublereal));

    gsav = global_;
    qsav = q_;
    dsav = d_;

    global_ = gstruct_alloc(sz);
    q_ = qstruct_alloc(sz);
    d_ = (double*) hoc_Ecalloc(sz, sizeof(double));


    *machep = 1e-11;
    /*                             LAST MODIFIED 3/1/73 */

    /*     PRAXIS RETURNS THE MINIMUM OF THE FUNCTION F(X,N) OF N VARIABLES */

    /*     USING THE PRINCIPAL AXIS METHOD.  THE GRADIENT OF THE FUNCTION IS
     */
    /*     NOT REQUIRED. */

    /*     FOR A DESCRIPTION OF THE ALGORITHM, SEE CHAPTER SEVEN OF */
    /*     "ALGORITHMS FOR FINDING ZEROS AND EXTREMA OF FUNCTIONS WITHOUT */
    /*     CALCULATING DERIVATIVES" BY RICHARD P BRENT. */

    /*     THE PARAMETERS ARE: */
    /*     T0       IS A TOLERANCE.  PRAXIS ATTEMPTS TO RETURN PRAXIS=F(X) */
    /*              SUCH THAT IF X0 IS THE TRUE LOCAL MINIMUM NEAR X, THEN */
    /*              NORM(X-X0) < T0 + SQUAREROOT(MACHEP)*NORM(X). */
    /*     MACHEP   IS THE MACHINE PRECISION, THE SMALLEST NUMBER SUCH THAT */

    /*              1 + MACHEP > 1.  MACHEP SHOULD BE 16.**-13 (ABOUT */
    /*              2.22D-16) FOR REAL*8 ARITHMETIC ON THE IBM 360. */
    /*     H0       IS THE MAXIMUM STEP SIZE.  H0 SHOULD BE SET TO ABOUT THE
     */
    /*              MAXIMUM DISTANCE FROM THE INITIAL GUESS TO THE MINIMUM. */

    /*              (IF H0 IS SET TOO LARGE OR TOO SMALL, THE INITIAL RATE OF
     */
    /*              CONVERGENCE MAY BE SLOW.) */
    /*     N        (AT LEAST TWO) IS THE NUMBER OF VARIABLES UPON WHICH */
    /*              THE FUNCTION DEPENDS. */
    /*     PRIN     CONTROLS THE PRINTING OF INTERMEDIATE RESULTS. */
    /*              IF PRIN=0, NOTHING IS PRINTED. */
    /*              IF PRIN=1, F IS PRINTED AFTER EVERY N+1 OR N+2 LINEAR */
    /*              MINIMIZATIONS.  FINAL X IS PRINTED, BUT INTERMEDIATE X IS
     */
    /*              PRINTED ONLY IF N IS AT MOST 4. */
    /*              IF PRIN=2, THE SCALE FACTORS AND THE PRINCIPAL VALUES OF
     */
    /*              THE APPROXIMATING QUADRATIC FORM ARE ALSO PRINTED. */
    /*              IF PRIN=3, X IS ALSO PRINTED AFTER EVERY FEW LINEAR */
    /*              MINIMIZATIONS. */
    /*              IF PRIN=4, THE PRINCIPAL VECTORS OF THE APPROXIMATING */
    /*              QUADRATIC FORM ARE ALSO PRINTED. */
    /*     X        IS AN ARRAY CONTAINING ON ENTRY A GUESS OF THE POINT OF */

    /*              MINIMUM, ON RETURN THE ESTIMATED POINT OF MINIMUM. */
    /*     F(X,N)   IS THE FUNCTION TO BE MINIMIZED.  F SHOULD BE A REAL*8 */
    /*              FUNCTION DECLARED EXTERNAL IN THE CALLING PROGRAM. */
    /*     FMIN     IS AN ESTIMATE OF THE MINIMUM, USED ONLY IN PRINTING */
    /*              INTERMEDIATE RESULTS. */
    /*     THE APPROXIMATING QUADRATIC FORM IS */
    /*              Q(X') = F(X,N) + (1/2) * (X'-X)-TRANSPOSE * A * (X'-X) */
    /*     WHERE X IS THE BEST ESTIMATE OF THE MINIMUM AND A IS */
    /*              INVERSE(V-TRANSPOSE) * D * INVERSE(V) */
    /*     (V(*,*) IS THE MATRIX OF SEARCH DIRECTIONS; D(*) IS THE ARRAY */
    /*     OF SECOND DIFFERENCES).  IF F HAS CONTINUOUS SECOND DERIVATIVES */
    /*     NEAR X0, A WILL TEND TO THE HESSIAN OF F AT X0 AS X APPROACHES X0.
     */

    /*     IT IS ASSUMED THAT ON FLOATING-POINT UNDERFLOW THE RESULT IS SET */

    /*     TO ZERO. */
    /*     THE USER SHOULD OBSERVE THE COMMENT ON HEURISTIC NUMBERS AFTER */
    /*     THE INITIALIZATION OF MACHINE DEPENDENT NUMBERS. */


    /* .....IF N>20 OR IF N<20 AND YOU NEED MORE SPACE, CHANGE '20' TO THE */
    /*     LARGEST VALUE OF N IN THE NEXT CARD, IN THE CARD 'IDIM=20', AND */
    /*     IN THE DIMENSION STATEMENTS IN SUBROUTINES MINFIT,MIN,FLIN,QUAD. */

    nval1 = nval;
    n = &nval1;

    /* .....INITIALIZATION..... */
    /*     MACHINE DEPENDENT NUMBERS: */

    /* Parameter adjustments */
    --x;

    /* Function Body */
    idim = sz;

    small = *machep * *machep;
    vsmall = small * small;
    large = 1. / small;
    vlarge = 1. / vsmall;
    m2 = sqrt(*machep);
    m4 = sqrt(m2);

    /*     HEURISTIC NUMBERS: */
    /*     IF THE AXES MAY BE BADLY SCALED (WHICH IS TO BE AVOIDED IF */
    /*     POSSIBLE), THEN SET SCBD=10.  OTHERWISE SET SCBD=1. */
    /*     IF THE PROBLEM IS KNOWN TO BE ILL-CONDITIONED, SET ILLC=TRUE. */
    /*     OTHERWISE SET ILLC=FALSE. */
    /*     KTM IS THE NUMBER OF ITERATIONS WITHOUT IMPROVEMENT BEFORE THE */
    /*     ALGORITHM TERMINATES.  KTM=4 IS VERY CAUTIOUS; USUALLY KTM=1 */
    /*     IS SATISFACTORY. */

    scbd = 1.;
    illc = FALSE_;
    ktm = 2;

    ldfac = .01;
    if (illc) {
        ldfac = .1;
    }
    kt = 0;
    global_1.nl = 0;
    global_1.nf = 1;
    global_1.fx = (*f)(&x[1], *n);
    if (stoprun) {
        goto ret_;
    }
    q_1.qf1 = global_1.fx;
    t = small + fabs(*t0);
    t2 = t;
    global_1.dmin_ = small;
    h = *h0;
    if (h < t * 100) {
        h = t * 100;
    }
    global_1.ldt = h;
    /* .....THE FIRST SET OF SEARCH DIRECTIONS V IS THE IDENTITY MATRIX.....
     */
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        i__2 = *n;
        for (j = 1; j <= i__2; ++j) {
            /* L10: */
            q_1.v[i + j * sz - sz1] = 0.;
        }
        /* L20: */
        q_1.v[i + i * sz - sz1] = 1.;
    }
    d[0] = 0.;
    q_1.qd0 = 0.;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        q_1.q0[i - 1] = x[i];
        /* L30: */
        q_1.q1[i - 1] = x[i];
    }
    if (*prin > 0) {
        print_(n, &x[1], prin, fmin);
    }
    if (*n <= 0) {
        goto L400;
    }
/* .....THE MAIN LOOP STARTS HERE..... */
L40:
    sf = d[0];
    d[0] = 0.;
    s = 0.;

    /* .....MINIMIZE ALONG THE FIRST DIRECTION V(*,1). */
    /*     FX MUST BE PASSED TO MIN BY VALUE. */
    value = global_1.fx;
    min_(n, &c__1, &c__2, d, &s, &value, &c__0, f, &x[1], &t, machep, &h);
    if (stoprun) {
        goto ret_;
    }

    if (s > 0.) {
        goto L50;
    }
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        /* L45: */
        q_1.v[i - 1] = -q_1.v[i - 1];
    }
L50:
    if (sf > d[0] * .9 && sf * .9 < d[0]) {
        goto L70;
    }
    i__1 = *n;
    for (i = 2; i <= i__1; ++i) {
        /* L60: */
        d[i - 1] = 0.;
    }

/* .....THE INNER LOOP STARTS HERE..... */
L70:
    if (*n > 1) {
        i__1 = *n;
        for (k = 2; k <= i__1; ++k) {
            i__2 = *n;
            for (i = 1; i <= i__2; ++i) {
                /* L75: */
                y[i - 1] = x[i];
            }
            sf = global_1.fx;
            if (kt > 0) {
                illc = TRUE_;
            }
        L80:
            kl = k;
            df = 0.;

            /* .....A RANDOM STEP FOLLOWS (TO AVOID RESOLUTION VALLEYS). */
            /*     PRAXIS ASSUMES THAT RANDOM RETURNS A RANDOM NUMBER UNIFORMLY */

            /*     DISTRIBUTED IN (0,1). */

            if (!illc) {
                goto L95;
            }
            i__2 = *n;
            for (i = 1; i <= i__2; ++i) {
                s = (global_1.ldt * .1 + t2 * pow(10., (double) kt)) * (random_(n) - .5);
                z[i - 1] = s;
                i__3 = *n;
                for (j = 1; j <= i__3; ++j) {
                    /* L85: */
                    x[j] += s * q_1.v[j + i * sz - sz1];
                }
                /* L90: */
            }
            global_1.fx = (*f)(&x[1], *n);
            if (stoprun) {
                goto ret_;
            }
            ++global_1.nf;

            /* .....MINIMIZE ALONG THE "NON-CONJUGATE" DIRECTIONS V(*,K),...,V(*,N
            ) */

        L95:
            i__2 = *n;
            for (k2 = k; k2 <= i__2; ++k2) {
                sl = global_1.fx;
                s = 0.;
                value = global_1.fx;
                min_(n, &k2, &c__2, &d[k2 - 1], &s, &value, &c__0, f, &x[1], &t, machep, &h);
                if (stoprun) {
                    goto ret_;
                }
                if (illc) {
                    goto L97;
                }
                s = sl - global_1.fx;
                goto L99;
            L97:
                /* Computing 2nd power */
                d__1 = s + z[k2 - 1];
                s = d[k2 - 1] * (d__1 * d__1);
            L99:
                if (df > s) {
                    goto L105;
                }
                df = s;
                kl = k2;
            L105:;
            }
            if (illc || df >= (d__1 = *machep * 100 * global_1.fx, fabs(d__1))) {
                goto L110;
            }

            /* .....IF THERE WAS NOT MUCH IMPROVEMENT ON THE FIRST TRY, SET */
            /*     ILLC=TRUE AND START THE INNER LOOP AGAIN..... */

            illc = TRUE_;
            goto L80;
        L110:
            if (k == 2 && *prin > 1) {
                vcprnt_(&c__1, d, n);
            }

            /* .....MINIMIZE ALONG THE "CONJUGATE" DIRECTIONS V(*,1),...,V(*,K-1)
             */

            km1 = k - 1;
            i__2 = km1;
            for (k2 = 1; k2 <= i__2; ++k2) {
                s = 0.;
                value = global_1.fx;
                min_(n, &k2, &c__2, &d[k2 - 1], &s, &value, &c__0, f, &x[1], &t, machep, &h);
                if (stoprun) {
                    goto ret_;
                }
                /* L120: */
            }
            f1 = global_1.fx;
            global_1.fx = sf;
            lds = 0.;
            i__2 = *n;
            for (i = 1; i <= i__2; ++i) {
                sl = x[i];
                x[i] = y[i - 1];
                sl -= y[i - 1];
                y[i - 1] = sl;
                /* L130: */
                lds += sl * sl;
            }
            lds = sqrt(lds);
            if (lds <= small) {
                goto L160;
            }

            /* .....DISCARD DIRECTION V(*,KL). */
            /*     IF NO RANDOM STEP WAS TAKEN, V(*,KL) IS THE "NON-CONJUGATE" */
            /*     DIRECTION ALONG WHICH THE GREATEST IMPROVEMENT WAS MADE..... */


            klmk = kl - k;
            if (klmk < 1) {
                goto L141;
            }
            i__2 = klmk;
            for (ii = 1; ii <= i__2; ++ii) {
                i = kl - ii;
                i__3 = *n;
                for (j = 1; j <= i__3; ++j) {
                    /* L135: */
                    q_1.v[j + (i + 1) * sz - sz1] = q_1.v[j + i * sz - sz1];
                }
                /* L140: */
                d[i] = d[i - 1];
            }
        L141:
            d[k - 1] = 0.;
            i__2 = *n;
            for (i = 1; i <= i__2; ++i) {
                /* L145: */
                q_1.v[i + k * sz - sz1] = y[i - 1] / lds;
            }

            /* .....MINIMIZE ALONG THE NEW "CONJUGATE" DIRECTION V(*,K), WHICH IS
             */
            /*     THE NORMALIZED VECTOR:  (NEW X) - (0LD X)..... */

            value = f1;
            min_(n, &k, &c__4, &d[k - 1], &lds, &value, &c__1, f, &x[1], &t, machep, &h);
            if (stoprun) {
                goto ret_;
            }
            if (lds > 0.) {
                goto L160;
            }
            lds = -lds;
            i__2 = *n;
            for (i = 1; i <= i__2; ++i) {
                /* L150: */
                q_1.v[i + k * sz - sz1] = -q_1.v[i + k * sz - sz1];
            }
        L160:
            global_1.ldt = ldfac * global_1.ldt;
            if (global_1.ldt < lds) {
                global_1.ldt = lds;
            }
            if (*prin > 0) {
                print_(n, &x[1], prin, fmin);
            }
            t2 = 0.;
            i__2 = *n;
            for (i = 1; i <= i__2; ++i) {
                /* L165: */
                /* Computing 2nd power */
                d__1 = x[i];
                t2 += d__1 * d__1;
            }
            t2 = m2 * sqrt(t2) + t;

            /* .....SEE WHETHER THE LENGTH OF THE STEP TAKEN SINCE STARTING THE */

            /*     INNER LOOP EXCEEDS HALF THE TOLERANCE..... */

            if (global_1.ldt > t2 * (float) .5) {
                kt = -1;
            }
            ++kt;
            if (kt > ktm) {
                goto L400;
            }
            /* L170: */
        }
        /* .....THE INNER LOOP ENDS HERE. */

        /*     TRY QUADRATIC EXTRAPOLATION IN CASE WE ARE IN A CURVED VALLEY. */
        /* L171: */
        quad_(n, f, &x[1], &t, machep, &h);
        if (stoprun) {
            goto ret_;
        }
        dn = 0.;
        i__1 = *n;
        for (i = 1; i <= i__1; ++i) {
            d[i - 1] = 1. / sqrt(d[i - 1]);
            if (dn < d[i - 1]) {
                dn = d[i - 1];
            }
            /* L175: */
        }
        if (*prin > 3) {
            maprnt_(&c__1, q_1.v, &idim, n);
        }
        i__1 = *n;
        for (j = 1; j <= i__1; ++j) {
            s = d[j - 1] / dn;
            i__2 = *n;
            for (i = 1; i <= i__2; ++i) {
                /* L180: */
                q_1.v[i + j * sz - sz1] = s * q_1.v[i + j * sz - sz1];
            }
        }
        /* .....SCALE THE AXES TO TRY TO REDUCE THE CONDITION NUMBER..... */

        if (scbd <= 1.) {
            goto L200;
        }
        s = vlarge;
        i__2 = *n;
        for (i = 1; i <= i__2; ++i) {
            sl = 0.;
            i__1 = *n;
            for (j = 1; j <= i__1; ++j) {
                /* L182: */
                sl += q_1.v[i + j * sz - sz1] * q_1.v[i + j * sz - sz1];
            }
            z[i - 1] = sqrt(sl);
            if (z[i - 1] < m4) {
                z[i - 1] = m4;
            }
            if (s > z[i - 1]) {
                s = z[i - 1];
            }
            /* L185: */
        }
        i__2 = *n;
        for (i = 1; i <= i__2; ++i) {
            sl = s / z[i - 1];
            z[i - 1] = 1. / sl;
            if (z[i - 1] <= scbd) {
                goto L189;
            }
            sl = 1. / scbd;
            z[i - 1] = scbd;
        L189:
            i__1 = *n;
            for (j = 1; j <= i__1; ++j) {
                /* L190: */
                q_1.v[i + j * sz - sz1] = sl * q_1.v[i + j * sz - sz1];
            }
            /* L195: */
        }

        /* .....CALCULATE A NEW SET OF ORTHOGONAL DIRECTIONS BEFORE REPEATING */
        /*     THE MAIN LOOP. */
        /*     FIRST TRANSPOSE V FOR MINFIT: */

    L200:
        i__2 = *n;
        for (i = 2; i <= i__2; ++i) {
            im1 = i - 1;
            i__1 = im1;
            for (j = 1; j <= i__1; ++j) {
                s = q_1.v[i + j * sz - sz1];
                q_1.v[i + j * sz - sz1] = q_1.v[j + i * sz - sz1];
                /* L210: */
                q_1.v[j + i * sz - sz1] = s;
            }
            /* L220: */
        }

        /* .....CALL MINFIT TO FIND THE SINGULAR VALUE DECOMPOSITION OF V. */
        /*     THIS GIVES THE PRINCIPAL VALUES AND PRINCIPAL DIRECTIONS OF THE */
        /*     APPROXIMATING QUADRATIC FORM WITHOUT SQUARING THE CONDITION */
        /*     NUMBER..... */

        minfit_(&idim, n, machep, &vsmall, q_1.v, d, global_1.minfit_e);

        /* .....UNSCALE THE AXES..... */

        if (scbd <= 1.) {
            goto L250;
        }
        i__2 = *n;
        for (i = 1; i <= i__2; ++i) {
            s = z[i - 1];
            i__1 = *n;
            for (j = 1; j <= i__1; ++j) {
                /* L225: */
                q_1.v[i + j * sz - sz1] = s * q_1.v[i + j * sz - sz1];
            }
            /* L230: */
        }
        i__2 = *n;
        for (i = 1; i <= i__2; ++i) {
            s = 0.;
            i__1 = *n;
            for (j = 1; j <= i__1; ++j) {
                /* L235: */
                /* Computing 2nd power */
                d__1 = q_1.v[j + i * sz - sz1];
                s += d__1 * d__1;
            }
            s = sqrt(s);
            d[i - 1] = s * d[i - 1];
            s = 1 / s;
            i__1 = *n;
            for (j = 1; j <= i__1; ++j) {
                /* L240: */
                q_1.v[j + i * sz - sz1] = s * q_1.v[j + i * sz - sz1];
            }
            /* L245: */
        }

    L250:
        i__2 = *n;
        for (i = 1; i <= i__2; ++i) {
            dni = dn * d[i - 1];
            if (dni > large) {
                goto L265;
            }
            if (dni < small) {
                goto L260;
            }
            d[i - 1] = 1 / (dni * dni);
            goto L270;
        L260:
            d[i - 1] = vlarge;
            goto L270;
        L265:
            d[i - 1] = vsmall;
        L270:;
        }
        /* .....SORT THE EIGENVALUES AND EIGENVECTORS..... */

        sort_(&idim, n, d, q_1.v);
        global_1.dmin_ = d[*n - 1];
        if (global_1.dmin_ < small) {
            global_1.dmin_ = small;
        }
        illc = FALSE_;
        if (m2 * d[0] > global_1.dmin_) {
            illc = TRUE_;
        }
    } else {
        if (fabs(s) < t) {
            goto L400;
        }
    }
    if (*prin > 1 && scbd > 1.) {
        vcprnt_(&c__2, z, n);
    }
    if (*prin > 1) {
        vcprnt_(&c__3, d, n);
    }
    if (*prin > 3) {
        maprnt_(&c__2, q_1.v, &idim, n);
    }
    if (after_quad) {
        struct QStruct* qsav;
        double* dsav;
        qsav = q_hoc;
        q_hoc = q_;
        dsav = d_hoc;
        d_hoc = d_;
        hoc_after_prax_quad(after_quad);
        q_hoc = qsav;
        d_hoc = dsav;
    }
    if (--praxstep == 0) { /* allows analysis of approach to fit by */
        praxstep = 0;
        goto L400;
    }

    /* .....THE MAIN LOOP ENDS HERE..... */

    goto L40;

    /* .....RETURN..... */

L400:
    if (*prin > 0) {
        vcprnt_(&c__3, d, n);
        maprnt_(&c__2, q_1.v, &idim, n);
        vcprnt_(&c__4, &x[1], n);
    }

/* for re-entrancy */
ret_:
    ret_val = global_1.fx;
    gstruct_free(global_);
    global_ = gsav;
    if (q_hoc) {
        qstruct_free(q_hoc);
        free(d_hoc);
    }
    q_hoc = q_;
    d_hoc = d_;
    q_ = qsav;
    d_ = dsav;
    free(y);
    free(z);

    return ret_val;
} /* praxis_ */

#undef d

static /* Subroutine */ int minfit_(integer* m,
                                    integer* n,
                                    doublereal* machep,
                                    doublereal* tol,
                                    doublereal* ab,
                                    doublereal* q,
                                    doublereal* e) {
    /* System generated locals */
    integer ab_dim1, ab_offset, i__1, i__2, i__3;
    doublereal d__1, d__2;

    /* Builtin functions */

    /* Local variables */
    doublereal temp, c, f, g, h;
    integer i, j, k, l;
    doublereal s, x, y, z;
    integer l2, ii, kk, kt, ll2, lp1;
    doublereal eps;


    /* ...AN IMPROVED VERSION OF MINFIT (SEE GOLUB AND REINSCH, 1969) */
    /*   RESTRICTED TO M=N,P=0. */
    /*   THE SINGULAR VALUES OF THE ARRAY AB ARE RETURNED IN Q AND AB IS */
    /*   OVERWRITTEN WITH THE ORTHOGONAL MATRIX V SUCH THAT U.DIAG(Q) = AB.V,
     */
    /*   WHERE U IS ANOTHER ORTHOGONAL MATRIX. */
    /* ...HOUSEHOLDER'S REDUCTION TO BIDIAGONAL FORM... */
    /* Parameter adjustments */
    --q;
    ab_dim1 = *m;
    ab_offset = ab_dim1 + 1;
    ab -= ab_offset;

    /* Function Body */
    if (*n == 1) {
        goto L200;
    }
    eps = *machep;
    g = 0.;
    x = 0.;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        e[i - 1] = g;
        s = 0.;
        l = i + 1;
        i__2 = *n;
        for (j = i; j <= i__2; ++j) {
            /* L1: */
            /* Computing 2nd power */
            d__1 = ab[j + i * ab_dim1];
            s += d__1 * d__1;
        }
        g = 0.;
        if (s < *tol) {
            goto L4;
        }
        f = ab[i + i * ab_dim1];
        g = sqrt(s);
        if (f >= 0.) {
            g = -g;
        }
        h = f * g - s;
        ab[i + i * ab_dim1] = f - g;
        if (l > *n) {
            goto L4;
        }
        i__2 = *n;
        for (j = l; j <= i__2; ++j) {
            f = 0.;
            i__3 = *n;
            for (k = i; k <= i__3; ++k) {
                /* L2: */
                f += ab[k + i * ab_dim1] * ab[k + j * ab_dim1];
            }
            f /= h;
            i__3 = *n;
            for (k = i; k <= i__3; ++k) {
                /* L3: */
                ab[k + j * ab_dim1] += f * ab[k + i * ab_dim1];
            }
        }
    L4:
        q[i] = g;
        s = 0.;
        if (i == *n) {
            goto L6;
        }
        i__3 = *n;
        for (j = l; j <= i__3; ++j) {
            /* L5: */
            s += ab[i + j * ab_dim1] * ab[i + j * ab_dim1];
        }
    L6:
        g = 0.;
        if (s < *tol) {
            goto L10;
        }
        if (i == *n) {
            goto L16;
        }
        f = ab[i + (i + 1) * ab_dim1];
    L16:
        g = sqrt(s);
        if (f >= 0.) {
            g = -g;
        }
        h = f * g - s;
        if (i == *n) {
            goto L10;
        }
        ab[i + (i + 1) * ab_dim1] = f - g;
        i__3 = *n;
        for (j = l; j <= i__3; ++j) {
            /* L7: */
            e[j - 1] = ab[i + j * ab_dim1] / h;
        }
        i__3 = *n;
        for (j = l; j <= i__3; ++j) {
            s = 0.;
            i__2 = *n;
            for (k = l; k <= i__2; ++k) {
                /* L8: */
                s += ab[j + k * ab_dim1] * ab[i + k * ab_dim1];
            }
            i__2 = *n;
            for (k = l; k <= i__2; ++k) {
                /* L9: */
                ab[j + k * ab_dim1] += s * e[k - 1];
            }
        }
    L10:
        y = (d__1 = q[i], fabs(d__1)) + (d__2 = e[i - 1], fabs(d__2));
        /* L11: */
        if (y > x) {
            x = y;
        }
    }
    /* ...ACCUMULATION OF RIGHT-HAND TRANSFORMATIONS... */
    ab[*n + *n * ab_dim1] = 1.;
    g = e[*n - 1];
    l = *n;
    i__1 = *n;
    for (ii = 2; ii <= i__1; ++ii) {
        i = *n - ii + 1;
        if (g == 0.) {
            goto L23;
        }
        h = ab[i + (i + 1) * ab_dim1] * g;
        i__2 = *n;
        for (j = l; j <= i__2; ++j) {
            /* L20: */
            ab[j + i * ab_dim1] = ab[i + j * ab_dim1] / h;
        }
        i__2 = *n;
        for (j = l; j <= i__2; ++j) {
            s = 0.;
            i__3 = *n;
            for (k = l; k <= i__3; ++k) {
                /* L21: */
                s += ab[i + k * ab_dim1] * ab[k + j * ab_dim1];
            }
            i__3 = *n;
            for (k = l; k <= i__3; ++k) {
                /* L22: */
                ab[k + j * ab_dim1] += s * ab[k + i * ab_dim1];
            }
        }
    L23:
        i__3 = *n;
        for (j = l; j <= i__3; ++j) {
            ab[i + j * ab_dim1] = 0.;
            /* L24: */
            ab[j + i * ab_dim1] = 0.;
        }
        ab[i + i * ab_dim1] = 1.;
        g = e[i - 1];
        /* L25: */
        l = i;
    }
    /* ...DIAGONALIZATION OF THE BIDIAGONAL FORM... */
    /* L100: */
    eps *= x;
    i__1 = *n;
    for (kk = 1; kk <= i__1; ++kk) {
        k = *n - kk + 1;
        kt = 0;
    L101:
        ++kt;
        if (kt <= 30) {
            goto L102;
        }
        e[k - 1] = 0.;
        printf("qr failed\n");
    L102:
        i__3 = k;
        for (ll2 = 1; ll2 <= i__3; ++ll2) {
            l2 = k - ll2 + 1;
            l = l2;
            if ((d__1 = e[l - 1], fabs(d__1)) <= eps) {
                goto L120;
            }
            if (l == 1) {
                goto L103;
            }
            if ((d__1 = q[l - 1], fabs(d__1)) <= eps) {
                goto L110;
            }
        L103:;
        }
    /* ...CANCELLATION OF E(L) IF L>1... */
    L110:
        c = 0.;
        s = 1.;
        i__3 = k;
        for (i = l; i <= i__3; ++i) {
            f = s * e[i - 1];
            e[i - 1] = c * e[i - 1];
            if (fabs(f) <= eps) {
                goto L120;
            }
            g = q[i];
            /* ...Q(I) = H = DSQRT(G*G + F*F)... */
            if (fabs(f) < fabs(g)) {
                goto L113;
            }
            if (f != 0.) {
                goto L112;
            } else {
                goto L111;
            }
        L111:
            h = 0.;
            goto L114;
        L112:
            /* Computing 2nd power */
            d__1 = g / f;
            h = fabs(f) * sqrt(d__1 * d__1 + 1);
            goto L114;
        L113:
            /* Computing 2nd power */
            d__1 = f / g;
            h = fabs(g) * sqrt(d__1 * d__1 + 1);
        L114:
            q[i] = h;
            if (h != 0.) {
                goto L115;
            }
            g = 1.;
            h = 1.;
        L115:
            c = g / h;
            /* L116: */
            s = -f / h;
        }
    /* ...TEST FOR CONVERGENCE... */
    L120:
        z = q[k];
        if (l == k) {
            goto L140;
        }
        /* ...SHIFT FROM BOTTOM 2*2 MINOR... */
        x = q[l];
        y = q[k - 1];
        g = e[k - 2];
        h = e[k - 1];
        f = ((y - z) * (y + z) + (g - h) * (g + h)) / (h * 2 * y);
        g = sqrt(f * f + 1.);
        temp = f - g;
        if (f >= 0.) {
            temp = f + g;
        }
        f = ((x - z) * (x + z) + h * (y / temp - h)) / x;
        /* ...NEXT QR TRANSFORMATION... */
        c = 1.;
        s = 1.;
        lp1 = l + 1;
        if (lp1 > k) {
            goto L133;
        }
        i__3 = k;
        for (i = lp1; i <= i__3; ++i) {
            g = e[i - 1];
            y = q[i];
            h = s * g;
            g *= c;
            if (fabs(f) < fabs(h)) {
                goto L123;
            }
            if (f != 0.) {
                goto L122;
            } else {
                goto L121;
            }
        L121:
            z = 0.;
            goto L124;
        L122:
            /* Computing 2nd power */
            d__1 = h / f;
            z = fabs(f) * sqrt(d__1 * d__1 + 1);
            goto L124;
        L123:
            /* Computing 2nd power */
            d__1 = f / h;
            z = fabs(h) * sqrt(d__1 * d__1 + 1);
        L124:
            e[i - 2] = z;
            if (z != 0.) {
                goto L125;
            }
            f = 1.;
            z = 1.;
        L125:
            c = f / z;
            s = h / z;
            f = x * c + g * s;
            g = -x * s + g * c;
            h = y * s;
            y *= c;
            i__2 = *n;
            for (j = 1; j <= i__2; ++j) {
                x = ab[j + (i - 1) * ab_dim1];
                z = ab[j + i * ab_dim1];
                ab[j + (i - 1) * ab_dim1] = x * c + z * s;
                /* L126: */
                ab[j + i * ab_dim1] = -x * s + z * c;
            }
            if (fabs(f) < fabs(h)) {
                goto L129;
            }
            if (f != 0.) {
                goto L128;
            } else {
                goto L127;
            }
        L127:
            z = 0.;
            goto L130;
        L128:
            /* Computing 2nd power */
            d__1 = h / f;
            z = fabs(f) * sqrt(d__1 * d__1 + 1);
            goto L130;
        L129:
            /* Computing 2nd power */
            d__1 = f / h;
            z = fabs(h) * sqrt(d__1 * d__1 + 1);
        L130:
            q[i - 1] = z;
            if (z != 0.) {
                goto L131;
            }
            f = 1.;
            z = 1.;
        L131:
            c = f / z;
            s = h / z;
            f = c * g + s * y;
            /* L132: */
            x = -s * g + c * y;
        }
    L133:
        e[l - 1] = 0.;
        e[k - 1] = f;
        q[k] = x;
        goto L101;
    /* ...CONVERGENCE:  Q(K) IS MADE NON-NEGATIVE... */
    L140:
        if (z >= 0.) {
            goto L150;
        }
        q[k] = -z;
        i__3 = *n;
        for (j = 1; j <= i__3; ++j) {
            /* L141: */
            ab[j + k * ab_dim1] = -ab[j + k * ab_dim1];
        }
    L150:;
    }
    return 0;
L200:
    q[1] = ab[ab_dim1 + 1];
    ab[ab_dim1 + 1] = 1.;
    return 0;
} /* minfit_ */

static /* Subroutine */ int min_(integer* n,
                                 integer* j,
                                 integer* nits,
                                 doublereal* d2,
                                 doublereal* x1,
                                 doublereal* f1,
                                 logical* fk,
                                 doublereal (*f)(doublereal*, integer),
                                 doublereal* x,
                                 doublereal* t,
                                 doublereal* machep,
                                 doublereal* h) {
    /* System generated locals */
    integer i__1;
    doublereal d__1, d__2;

    /* Builtin functions */

    /* Local variables */
    doublereal temp;
    integer i, k;
    integer sz, sz1;
    doublereal s, small, d1, f0, f2, m2, m4, t2, x2, fm;
    logical dz;
    doublereal xm, sf1, sx1;

    /* ...THE SUBROUTINE MIN MINIMIZES F FROM X IN THE DIRECTION V(*,J) UNLESS
     */
    /*   J IS LESS THAN 1, WHEN A QUADRATIC SEARCH IS MADE IN THE PLANE */
    /*   DEFINED BY Q0,Q1,X. */
    /*   D2 IS EITHER ZERO OR AN APPROXIMATION TO HALF F". */
    /*   ON ENTRY, X1 IS AN ESTIMATE OF THE DISTANCE FROM X TO THE MINIMUM */
    /*   ALONG V(*,J) (OR, IF J=0, A CURVE).  ON RETURN, X1 IS THE DISTANCE */

    /*   FOUND. */
    /*   IF FK=.TRUE., THEN F1 IS FLIN(X1).  OTHERWISE X1 AND F1 ARE IGNORED
     */
    /*   ON ENTRY UNLESS FINAL FX IS GREATER THAN F1. */
    /*   NITS CONTROLS THE NUMBER OF TIMES AN ATTEMPT WILL BE MADE TO HALVE */

    /*   THE INTERVAL. */
    /* Parameter adjustments */
    --x;

    /* Function Body */
    sz = *n;
    sz1 = sz + 1;
    /* Computing 2nd power */
    d__1 = *machep;
    small = d__1 * d__1;
    m2 = sqrt(*machep);
    m4 = sqrt(m2);
    sf1 = *f1;
    sx1 = *x1;
    k = 0;
    xm = 0.;
    fm = global_1.fx;
    f0 = global_1.fx;
    dz = *d2 < *machep;
    /* ...FIND THE STEP SIZE... */
    s = 0.;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        /* L1: */
        /* Computing 2nd power */
        d__1 = x[i];
        s += d__1 * d__1;
    }
    s = sqrt(s);
    temp = *d2;
    if (dz) {
        temp = global_1.dmin_;
    }
    t2 = m4 * sqrt(fabs(global_1.fx) / temp + s * global_1.ldt) + m2 * global_1.ldt;
    s = m4 * s + *t;
    if (dz && t2 > s) {
        t2 = s;
    }
    t2 = dmax(t2, small);
    /* Computing MIN */
    d__1 = t2;
    d__2 = *h * .01;
    t2 = dmin(d__1, d__2);
    if (!(*fk) || *f1 > fm) {
        goto L2;
    }
    xm = *x1;
    fm = *f1;
L2:
    if (*fk && fabs(*x1) >= t2) {
        goto L3;
    }
    temp = 1.;
    if (*x1 < 0.) {
        temp = -1.;
    }
    *x1 = temp * t2;
    *f1 = flin_(n, j, x1, f, &x[1], &global_1.nf, global_1.flin_t);
    if (stoprun) {
        return global_1.fx;
    }
L3:
    if (*f1 > fm) {
        goto L4;
    }
    xm = *x1;
    fm = *f1;
L4:
    if (!dz) {
        goto L6;
    }
    /* ...EVALUATE FLIN AT ANOTHER POINT AND ESTIMATE THE SECOND DERIVATIVE...
     */
    x2 = -(*x1);
    if (f0 >= *f1) {
        x2 = *x1 * 2.;
    }
    f2 = flin_(n, j, &x2, f, &x[1], &global_1.nf, global_1.flin_t);
    if (stoprun) {
        return global_1.fx;
    }
    if (f2 > fm) {
        goto L5;
    }
    xm = x2;
    fm = f2;
L5:
    *d2 = (x2 * (*f1 - f0) - *x1 * (f2 - f0)) / (*x1 * x2 * (*x1 - x2));
/* ...ESTIMATE THE FIRST DERIVATIVE AT 0... */
L6:
    d1 = (*f1 - f0) / *x1 - *x1 * *d2;
    dz = TRUE_;
    /* ...PREDICT THE MINIMUM... */
    if (*d2 > small) {
        goto L7;
    }
    x2 = *h;
    if (d1 >= 0.) {
        x2 = -x2;
    }
    goto L8;
L7:
    x2 = d1 * -.5 / *d2;
L8:
    if (fabs(x2) <= *h) {
        goto L11;
    }
    if (x2 <= 0.) {
        goto L9;
    } else {
        goto L10;
    }
L9:
    x2 = -(*h);
    goto L11;
L10:
    x2 = *h;
/* ...EVALUATE F AT THE PREDICTED MINIMUM... */
L11:
    f2 = flin_(n, j, &x2, f, &x[1], &global_1.nf, global_1.flin_t);
    if (stoprun) {
        return global_1.fx;
    }
    if (k >= *nits || f2 <= f0) {
        goto L12;
    }
    /* ...NO SUCCESS, SO TRY AGAIN... */
    ++k;
    if (f0<*f1&& * x1 * x2> 0.) {
        goto L4;
    }
    x2 *= .5;
    goto L11;
/* ...INCREMENT THE ONE-DIMENSIONAL SEARCH COUNTER... */
L12:
    ++global_1.nl;
    if (f2 <= fm) {
        goto L13;
    }
    x2 = xm;
    goto L14;
L13:
    fm = f2;
/* ...GET A NEW ESTIMATE OF THE SECOND DERIVATIVE... */
L14:
    if ((d__1 = x2 * (x2 - *x1), fabs(d__1)) <= small) {
        goto L15;
    }
    *d2 = (x2 * (*f1 - f0) - *x1 * (fm - f0)) / (*x1 * x2 * (*x1 - x2));
    goto L16;
L15:
    if (k > 0) {
        *d2 = 0.;
    }
L16:
    if (*d2 <= small) {
        *d2 = small;
    }
    *x1 = x2;
    global_1.fx = fm;
    if (sf1 >= global_1.fx) {
        goto L17;
    }
    global_1.fx = sf1;
    *x1 = sx1;
/* ...UPDATE X FOR LINEAR BUT NOT PARABOLIC SEARCH... */
L17:
    if (*j == 0) {
        return 0;
    }
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        /* L18: */
        x[i] += *x1 * q_1.v[i + *j * sz - sz1];
    }
    return 0;
} /* min_ */

static doublereal flin_(integer* n,
                        integer* j,
                        doublereal* l,
                        doublereal (*f)(doublereal*, integer),
                        doublereal* x,
                        integer* nf,
                        doublereal* t) {
    /* System generated locals */
    integer i__1;
    doublereal ret_val;

    /* Local variables */
    integer i;
    integer sz, sz1;

    /* ...FLIN IS THE FUNCTION OF ONE REAL VARIABLE L THAT IS MINIMIZED */
    /*   BY THE SUBROUTINE MIN... */
    /* Parameter adjustments */
    --x;

    /* Function Body */
    if (*j == 0) {
        goto L2;
    }
    /* ...THE SEARCH IS LINEAR... */
    i__1 = *n;
    sz = i__1;
    sz1 = sz + 1;
    for (i = 1; i <= i__1; ++i) {
        /* L1: */
        t[i - 1] = x[i] + *l * q_1.v[i + *j * sz - sz1];
    }
    goto L4;
/* ...THE SEARCH IS ALONG A PARABOLIC SPACE CURVE... */
L2:
    q_1.qa = *l * (*l - q_1.qd1) / (q_1.qd0 * (q_1.qd0 + q_1.qd1));
    q_1.qb = (*l + q_1.qd0) * (q_1.qd1 - *l) / (q_1.qd0 * q_1.qd1);
    q_1.qc = *l * (*l + q_1.qd0) / (q_1.qd1 * (q_1.qd0 + q_1.qd1));
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        /* L3: */
        t[i - 1] = q_1.qa * q_1.q0[i - 1] + q_1.qb * x[i] + q_1.qc * q_1.q1[i - 1];
    }
/* ...THE FUNCTION EVALUATION COUNTER NF IS INCREMENTED... */
L4:
    ++(*nf);
    ret_val = (*f)(t, *n);
    return ret_val;
} /* flin_ */

static /* Subroutine */ int sort_(integer* m, integer* n, doublereal* d, doublereal* v) {
    /* System generated locals */
    integer v_dim1, v_offset, i__1, i__2;

    /* Local variables */
    integer i, j, k;
    doublereal s;
    integer ip1, nm1;

    /* ...SORTS THE ELEMENTS OF D(N) INTO DESCENDING ORDER AND MOVES THE */
    /*   CORRESPONDING COLUMNS OF V(N,N). */
    /*   M IS THE ROW DIMENSION OF V AS DECLARED IN THE CALLING PROGRAM. */
    /* Parameter adjustments */
    v_dim1 = *m;
    v_offset = v_dim1 + 1;
    v -= v_offset;
    --d;

    /* Function Body */
    if (*n == 1) {
        return 0;
    }
    nm1 = *n - 1;
    i__1 = nm1;
    for (i = 1; i <= i__1; ++i) {
        k = i;
        s = d[i];
        ip1 = i + 1;
        i__2 = *n;
        for (j = ip1; j <= i__2; ++j) {
            if (d[j] <= s) {
                goto L1;
            }
            k = j;
            s = d[j];
        L1:;
        }
        if (k <= i) {
            goto L3;
        }
        d[k] = d[i];
        d[i] = s;
        i__2 = *n;
        for (j = 1; j <= i__2; ++j) {
            s = v[j + i * v_dim1];
            v[j + i * v_dim1] = v[j + k * v_dim1];
            /* L2: */
            v[j + k * v_dim1] = s;
        }
    L3:;
    }
    return 0;
} /* sort_ */

static /* Subroutine */ int quad_(integer* n,
                                  doublereal (*f)(doublereal*, integer),
                                  doublereal* x,
                                  doublereal* t,
                                  doublereal* machep,
                                  doublereal* h) {
    /* System generated locals */
    integer i__1;
    doublereal d__1;

    /* Builtin functions */

    /* Local variables */
    integer i;
    doublereal l, s, value;

    /* ...QUAD LOOKS FOR THE MINIMUM OF F ALONG A CURVE DEFINED BY Q0,Q1,X...
     */
    /* Parameter adjustments */
    --x;

    /* Function Body */
    s = global_1.fx;
    global_1.fx = q_1.qf1;
    q_1.qf1 = s;
    q_1.qd1 = 0.;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        s = x[i];
        l = q_1.q1[i - 1];
        x[i] = l;
        q_1.q1[i - 1] = s;
        /* L1: */
        /* Computing 2nd power */
        d__1 = s - l;
        q_1.qd1 += d__1 * d__1;
    }
    q_1.qd1 = sqrt(q_1.qd1);
    l = q_1.qd1;
    s = 0.;
    if (q_1.qd0 <= 0. || q_1.qd1 <= 0. || global_1.nl < *n * 3 * *n) {
        goto L2;
    }
    value = q_1.qf1;
    min_(n, &c__0, &c__2, &s, &l, &value, &c__1, f, &x[1], t, machep, h);
    if (stoprun) {
        return global_1.fx;
    }
    q_1.qa = l * (l - q_1.qd1) / (q_1.qd0 * (q_1.qd0 + q_1.qd1));
    q_1.qb = (l + q_1.qd0) * (q_1.qd1 - l) / (q_1.qd0 * q_1.qd1);
    q_1.qc = l * (l + q_1.qd0) / (q_1.qd1 * (q_1.qd0 + q_1.qd1));
    goto L3;
L2:
    global_1.fx = q_1.qf1;
    q_1.qa = 0.;
    q_1.qb = q_1.qa;
    q_1.qc = 1.;
L3:
    q_1.qd0 = q_1.qd1;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        s = q_1.q0[i - 1];
        q_1.q0[i - 1] = x[i];
        /* L4: */
        x[i] = q_1.qa * s + q_1.qb * x[i] + q_1.qc * q_1.q1[i - 1];
    }
    return 0;
} /* quad_ */

static /* Subroutine */ int vcprnt_(integer* option, doublereal* v, integer* n) {
    /* System generated locals */
    integer i__1;

    /* Local variables */
    integer i;


    /* Parameter adjustments */
    --v;

    /* Function Body */
    switch ((int) *option) {
    case 1:
        goto L1;
    case 2:
        goto L2;
    case 3:
        goto L3;
    case 4:
        goto L4;
    }
L1:
    printf("The second difference array d[*] is :\n");
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        printf("%g\n", v[i]);
    }
    return 0;
L2:
    printf("The scale factors are:\n");
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        printf("%g\n", v[i]);
    }
    return 0;
L3:
    printf("The approximating quadratic form has the principal values:\n");
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        printf("%g\n", v[i]);
    }
    return 0;
L4:
    printf("x is:\n");
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        printf("%g\n", v[i]);
    }
    return 0;
} /* vcprnt_ */

static /* Subroutine */ int print_(integer* n, doublereal* x, integer* prin, doublereal* fmin) {
    /* System generated locals */
    integer i__1;
    doublereal d__1;

    /* Local variables */
    integer i;
    doublereal ln;


    /* Parameter adjustments */
    --x;

    /* Function Body */
    printf("After ");
    printf("%ld", global_1.nl);
    printf(" linear searches, the function has been evaluated ");
    printf("%ld times.\n", global_1.nf);
    printf("The smallest value found is f(x) = ");
    printf("%g\n", global_1.fx);
    if (global_1.fx <= *fmin) {
        goto L1;
    }
    d__1 = global_1.fx - *fmin;
    ln = log10(d__1);
    printf("log (f(x)) - ");
    printf("%g", *fmin);
    printf(" = ");
    printf("%g\n", ln);
    goto L2;
L1:
    printf("log (f(x)) -- ");
    printf("%g", *fmin);
    printf(" is undefined\n");
L2:
    if (*n > 4 && *prin <= 2) {
        return 0;
    }
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        printf("x is:");
        printf("%g\n", x[i]);
    }
    return 0;
} /* print_ */

static /* Subroutine */ int maprnt_(integer* option, doublereal* v, integer* m, integer* n) {
    /* System generated locals */
    integer v_dim1, v_offset, i__1, i__2;

    /* Local variables */
    integer i, j, low, upp;


    /* ...THE SUBROUTINE MAPRNT PRINTS THE COLUMNS OF THE NXN MATRIX V */
    /*   WITH A HEADING AS SPECIFIED BY OPTION. */
    /*   M IS THE ROW DIMENSION OF V AS DECLARED IN THE CALLING PROGRAM... */
    /* Parameter adjustments */
    v_dim1 = *m;
    v_offset = v_dim1 + 1;
    v -= v_offset;

    /* Function Body */
    low = 1;
    upp = 5;
    switch ((int) *option) {
    case 1:
        goto L1;
    case 2:
        goto L2;
    }
L1:
    printf("The new directions are:\n");
    goto L3;
L2:
    printf("and the principal axes:\n");
L3:
    if (*n < upp) {
        upp = *n;
    }
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
        /* L4: */
        printf("%3ld", i);
        i__2 = upp;
        for (j = low; j <= i__2; ++j) {
            printf("  %12g", v[i * v_dim1 + j]);
        }
        printf("\n");
    }
    low += 5;
    if (*n < low) {
        return 0;
    }
    upp += 5;
    goto L3;
} /* maprnt_ */

uint32_t nrn_praxis_ran_index;

static doublereal random_(integer* naught) {
    double x;
    return mcell_ran4(&nrn_praxis_ran_index, &x, 1, 1.);
#if 0
    /* Initialized data */

    static logical init = FALSE_;

    /* System generated locals */
    doublereal ret_val;

    /* Local variables */
    static doublereal half;
    static integer i, j, q, r;
    static doublereal ran1;
    static integer ran2;
    static doublereal ran3[127];

    if (init) {
	goto L3;
    }
    r = *naught % 8190 + 1;
    ran2 = 128;
    for (i = 1; i <= 127; ++i) {
	--ran2;
	ran1 = -36028797018963968.;
	for (j = 1; j <= 7; ++j) {
	    r = r * 1756 % 8191;
	    q = r / 32;
/* L1: */
	    ran1 = (ran1 + q) * .00390625;
	}
/* L2: */
	ran3[ran2 - 1] = ran1;
    }
    init = TRUE_;
L3:
    if (ran2 == 1) {
	ran2 = 128;
    }
    --ran2;
    ran1 += ran3[ran2 - 1];
    half = .5;
    if (ran1 >= 0.) {
	half = -half;
    }
    ran1 += half;
    ran3[ran2 - 1] = ran1;
    ret_val = ran1 + .5;
    return ret_val;
#endif
} /* random_ */

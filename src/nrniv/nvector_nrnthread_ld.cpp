/*
 * -----------------------------------------------------------------
 * $Revision: 855 $
 * $Date: 2005-02-09 18:15:46 -0500 (Wed, 09 Feb 2005) $
 * -----------------------------------------------------------------
 * Programmer(s): Scott D. Cohen, Alan C. Hindmarsh, Radu Serban,
 *                and Aaron Collier @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California.
 * Produced at the Lawrence Livermore National Laboratory.
 * All rights reserved.
 * For details, see sundials/shared/LICENSE.
 * -----------------------------------------------------------------
 * This is the implementation file for a nrnthread implementation
 * of the NVECTOR package.
 * -----------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>

#include "shared/nvector_serial.h"
#include "nvector_nrnthread_ld.h"
#include "shared/sundialsmath.h"
#include "shared/sundialstypes.h"
#include "section.h"
#include "nrnmutdec.h"

#define ZERO   RCONST(0.0)
#define HALF   RCONST(0.5)
#define ONE    RCONST(1.0)
#define ONEPT5 RCONST(1.5)

#if 0
#define mydebug(a)     printf(a)
#define mydebug2(a, b) printf(a, b)
#else
#define mydebug(a)     /**/
#define mydebug2(a, b) /**/
#endif

#if NRN_ENABLE_THREADS
static MUTDEC
#endif
/* argument passing between NrnThreadLD and Serial */
static N_Vector x_;
static N_Vector y_;
static N_Vector z_;
static N_Vector w_;
static N_Vector id_;
static realtype a_;
static realtype b_;
static realtype c_;
static realtype retval;
// for use alongside retval in Kahan summation that replaces old long double code
static realtype retval_comp;
static booleantype bretval;
#define xpass    x_ = x;
#define ypass    y_ = y;
#define zpass    z_ = z;
#define wpass    w_ = w;
#define idpass   id_ = id;
#define apass    a_ = a;
#define bpass    b_ = b;
#define cpass    c_ = c;
#define xarg(i)  NV_SUBVEC_NT_LD(x_, i)
#define yarg(i)  NV_SUBVEC_NT_LD(y_, i)
#define zarg(i)  NV_SUBVEC_NT_LD(z_, i)
#define warg(i)  NV_SUBVEC_NT_LD(w_, i)
#define idarg(i) NV_SUBVEC_NT_LD(id_, i)
#define aarg     a_
#define barg     b_
#define carg     c_
#define lock     MUTLOCK
#define unlock   MUTUNLOCK
#define lockadd(arg) \
    lock;            \
    retval += arg;   \
    unlock;
#define lockmax(arg)    \
    lock;               \
    if (retval < arg) { \
        retval = arg;   \
    };                  \
    unlock;
#define lockmin(arg)    \
    lock;               \
    if (retval > arg) { \
        retval = arg;   \
    };                  \
    unlock;
#define lockfalse    \
    lock;            \
    bretval = FALSE; \
    unlock;

/*
 * -----------------------------------------------------------------
 * exported functions
 * -----------------------------------------------------------------
 */

/* ----------------------------------------------------------------------------
 * Function to create a new empty nrnthread vector
 */

N_Vector N_VNewEmpty_NrnThreadLD(long int length, int nthread, long int* sizes) {
    int i;
    N_Vector v;
    N_Vector_Ops ops;
    N_VectorContent_NrnThreadLD content;

    if (!MUTCONSTRUCTED) {
        MUTCONSTRUCT(1)
    }

    /* Create vector */
    v = (N_Vector) malloc(sizeof *v);
    if (v == NULL)
        return (NULL);

    /* Create vector operation structure */
    ops = (N_Vector_Ops) malloc(sizeof(struct _generic_N_Vector_Ops));
    if (ops == NULL) {
        free(v);
        return (NULL);
    }

    ops->nvclone = N_VClone_NrnThreadLD;
    ops->nvdestroy = N_VDestroy_NrnThreadLD;
    ops->nvspace = N_VSpace_NrnThreadLD;
    ops->nvgetarraypointer = N_VGetArrayPointer_NrnThreadLD;
    ops->nvsetarraypointer = N_VSetArrayPointer_NrnThreadLD;
    ops->nvlinearsum = N_VLinearSum_NrnThreadLD;
    ops->nvconst = N_VConst_NrnThreadLD;
    ops->nvprod = N_VProd_NrnThreadLD;
    ops->nvdiv = N_VDiv_NrnThreadLD;
    ops->nvscale = N_VScale_NrnThreadLD;
    ops->nvabs = N_VAbs_NrnThreadLD;
    ops->nvinv = N_VInv_NrnThreadLD;
    ops->nvaddconst = N_VAddConst_NrnThreadLD;
    ops->nvdotprod = N_VDotProd_NrnThreadLD;
    ops->nvmaxnorm = N_VMaxNorm_NrnThreadLD;
    ops->nvwrmsnormmask = N_VWrmsNormMask_NrnThreadLD;
    ops->nvwrmsnorm = N_VWrmsNorm_NrnThreadLD;
    ops->nvmin = N_VMin_NrnThreadLD;
    ops->nvwl2norm = N_VWL2Norm_NrnThreadLD;
    ops->nvl1norm = N_VL1Norm_NrnThreadLD;
    ops->nvcompare = N_VCompare_NrnThreadLD;
    ops->nvinvtest = N_VInvTest_NrnThreadLD;
    ops->nvconstrmask = N_VConstrMask_NrnThreadLD;
    ops->nvminquotient = N_VMinQuotient_NrnThreadLD;

    /* Create content */
    content = (N_VectorContent_NrnThreadLD) malloc(sizeof(struct _N_VectorContent_NrnThreadLD));
    if (content == NULL) {
        free(ops);
        free(v);
        return (NULL);
    }

    content->length = length;
    content->nt = nthread;
    content->own_data = FALSE;
    content->data = (N_Vector*) malloc(sizeof(N_Vector) * nthread);
    if (content->data == NULL) {
        free(ops);
        free(v);
        free(content);
        return (NULL);
    }
    for (i = 0; i < nthread; ++i) {
        content->data[i] = NULL;
    }
    /* Attach content and ops */
    v->content = content;
    v->ops = ops;

    return (v);
}

/* ----------------------------------------------------------------------------
 * Function to create a new nrnthread vector
 */

N_Vector N_VNew_NrnThreadLD(long int length, int nthread, long int* sizes) {
    int i;
    N_Vector v;
    N_Vector data;
    N_VectorContent_NrnThreadLD* content;

    v = N_VNewEmpty_NrnThreadLD(length, nthread, sizes);
    if (v == NULL)
        return (NULL);

    /* Create data */
    if (length > 0) {
        /* Allocate memory */
        NV_OWN_DATA_NT_LD(v) = TRUE;
        for (i = 0; i < nthread; ++i) {
            data = N_VNew_Serial(sizes[i]);
            if (data == NULL) {
                N_VDestroy_NrnThreadLD(v);
                return (NULL);
            }
            NV_SUBVEC_NT_LD(v, i) = data;
        }
    }

    return (v);
}

/* ----------------------------------------------------------------------------
 * Function to clone from a template a new vector with empty (NULL) data array
 */

N_Vector N_VCloneEmpty_NrnThreadLD(N_Vector w) {
    int i;
    N_Vector v;
    N_Vector_Ops ops;
    N_VectorContent_NrnThreadLD content;
    N_VectorContent_NrnThreadLD wcontent;

    if (w == NULL)
        return (NULL);

    /* Create vector */
    v = (N_Vector) malloc(sizeof *v);
    if (v == NULL)
        return (NULL);

    /* Create vector operation structure */
    ops = (N_Vector_Ops) malloc(sizeof(struct _generic_N_Vector_Ops));
    if (ops == NULL) {
        free(v);
        return (NULL);
    }

    ops->nvclone = w->ops->nvclone;
    ops->nvdestroy = w->ops->nvdestroy;
    ops->nvspace = w->ops->nvspace;
    ops->nvgetarraypointer = w->ops->nvgetarraypointer;
    ops->nvsetarraypointer = w->ops->nvsetarraypointer;
    ops->nvlinearsum = w->ops->nvlinearsum;
    ops->nvconst = w->ops->nvconst;
    ops->nvprod = w->ops->nvprod;
    ops->nvdiv = w->ops->nvdiv;
    ops->nvscale = w->ops->nvscale;
    ops->nvabs = w->ops->nvabs;
    ops->nvinv = w->ops->nvinv;
    ops->nvaddconst = w->ops->nvaddconst;
    ops->nvdotprod = w->ops->nvdotprod;
    ops->nvmaxnorm = w->ops->nvmaxnorm;
    ops->nvwrmsnormmask = w->ops->nvwrmsnormmask;
    ops->nvwrmsnorm = w->ops->nvwrmsnorm;
    ops->nvmin = w->ops->nvmin;
    ops->nvwl2norm = w->ops->nvwl2norm;
    ops->nvl1norm = w->ops->nvl1norm;
    ops->nvcompare = w->ops->nvcompare;
    ops->nvinvtest = w->ops->nvinvtest;
    ops->nvconstrmask = w->ops->nvconstrmask;
    ops->nvminquotient = w->ops->nvminquotient;

    /* Create content */
    content = (N_VectorContent_NrnThreadLD) malloc(sizeof(struct _N_VectorContent_NrnThreadLD));
    if (content == NULL) {
        free(ops);
        free(v);
        return (NULL);
    }

    wcontent = NV_CONTENT_NT_LD(w);
    content->length = NV_LENGTH_NT_LD(w);
    content->own_data = FALSE;
    content->nt = wcontent->nt;
    content->data = (N_Vector*) malloc(sizeof(N_Vector) * content->nt);
    if (content->data == NULL) {
        free(ops);
        free(v);
        free(content);
        return (NULL);
    }
    for (i = 0; i < content->nt; ++i) {
        content->data[i] = NULL;
    }

    /* Attach content and ops */
    v->content = content;
    v->ops = ops;

    return (v);
}

/* ----------------------------------------------------------------------------
 * Function to create a nrnthread N_Vector with user data component
 */

N_Vector N_VMake_NrnThreadLD(long int length, realtype* v_data) {
    N_Vector v = NULL;

    assert(0);
#if 0
  v = N_VNewEmpty_NrnThreadLD(length);
  if (v == NULL) return(NULL);

  if (length > 0) {
    /* Attach data */
    NV_OWN_DATA_NT_LD(v) = FALSE;
    NV_DATA_NT_LD(v) = v_data;
  }
#endif
    return (v);
}

/* ----------------------------------------------------------------------------
 * Function to create an array of new nrnthread vectors.
 */

N_Vector* N_VNewVectorArray_NrnThreadLD(int count, long int length, int nthread, long int* sizes) {
    N_Vector* vs;
    int j;

    if (count <= 0)
        return (NULL);

    vs = (N_Vector*) malloc(count * sizeof(N_Vector));
    if (vs == NULL)
        return (NULL);

    for (j = 0; j < count; j++) {
        vs[j] = N_VNew_NrnThreadLD(length, nthread, sizes);
        if (vs[j] == NULL) {
            N_VDestroyVectorArray_NrnThreadLD(vs, j - 1);
            return (NULL);
        }
    }

    return (vs);
}

/* ----------------------------------------------------------------------------
 * Function to create an array of new nrnthread vectors with NULL data array.
 */

N_Vector* N_VNewVectorArrayEmpty_NrnThreadLD(int count,
                                             long int length,
                                             int nthread,
                                             long int* sizes) {
    N_Vector* vs;
    int j;

    if (count <= 0)
        return (NULL);

    vs = (N_Vector*) malloc(count * sizeof(N_Vector));
    if (vs == NULL)
        return (NULL);

    for (j = 0; j < count; j++) {
        vs[j] = N_VNewEmpty_NrnThreadLD(length, nthread, sizes);
        if (vs[j] == NULL) {
            N_VDestroyVectorArray_NrnThreadLD(vs, j - 1);
            return (NULL);
        }
    }

    return (vs);
}

/* ----------------------------------------------------------------------------
 * Function to free an array created with N_VNewVectorArray_NrnThreadLD
 */

void N_VDestroyVectorArray_NrnThreadLD(N_Vector* vs, int count) {
    int j;

    for (j = 0; j < count; j++)
        N_VDestroy_NrnThreadLD(vs[j]);

    free(vs);
}

/* ----------------------------------------------------------------------------
 * Function to print the a nrnthread vector
 */

void N_VPrint_NrnThreadLD(N_Vector x) {
    int i;
    int nt;

    nt = NV_NT_NT_LD(x);

    for (i = 0; i < nt; i++) {
        N_VPrint_Serial(NV_SUBVEC_NT_LD(x, i));
    }
    printf("\n");
}

static void pr(N_Vector x) {
    N_VPrint_NrnThreadLD(x);
}
/*
 * -----------------------------------------------------------------
 * implementation of vector operations
 * -----------------------------------------------------------------
 */

N_Vector N_VClone_NrnThreadLD(N_Vector w) {
    N_Vector v;
    N_Vector wdata;
    N_Vector data;
    long int length;
    int i, nt;

    v = N_VCloneEmpty_NrnThreadLD(w);
    if (v == NULL)
        return (NULL);

    length = NV_LENGTH_NT_LD(w);
    nt = NV_NT_NT_LD(w);

    /* Create data */
    if (length > 0) {
        NV_OWN_DATA_NT_LD(v) = TRUE;
        for (i = 0; i < nt; ++i) {
            wdata = NV_SUBVEC_NT_LD(w, i);
            data = N_VClone(wdata);
            if (data == NULL) {
                N_VDestroy_NrnThreadLD(v);
                return (NULL);
            }
            NV_SUBVEC_NT_LD(v, i) = data;
        }
        /* Attach data */
    }

    return (v);
}

void N_VDestroy_NrnThreadLD(N_Vector v) {
    int i, nt;
    N_Vector data;
    nt = NV_NT_NT_LD(v);
    if (NV_OWN_DATA_NT_LD(v) == TRUE) {
        if (NV_CONTENT_NT_LD(v)->data) {
            for (i = 0; i < nt; ++i) {
                data = NV_SUBVEC_NT_LD(v, i);
                if (data) {
                    N_VDestroy(data);
                }
            }
            free(NV_CONTENT_NT_LD(v)->data);
        }
    }
    free(v->content);
    free(v->ops);
    free(v);
}

void N_VSpace_NrnThreadLD(N_Vector v, long int* lrw, long int* liw) {
    *lrw = NV_LENGTH_NT_LD(v);
    *liw = 1;
}

/* NOTICE: the pointer returned is actually the NVector* data where
data is nthread NVector. so when you get the realtype* cast it back to
(NVector*)
*/
realtype* N_VGetArrayPointer_NrnThreadLD(N_Vector v) {
    N_Vector* v_data;
    v_data = NV_DATA_NT_LD(v);

    return ((realtype*) v_data);
}

void N_VSetArrayPointer_NrnThreadLD(realtype* v_data, N_Vector v) {
    assert(0);
#if 0
  if (NV_LENGTH_NT_LD(v) > 0) NV_DATA_NT_LD(v) = v_data;
#endif
}

static void* vlinearsum(NrnThread* nt) {
    int i = nt->id;
    N_VLinearSum_Serial(aarg, xarg(i), barg, yarg(i), zarg(i));
    return nullptr;
}
void N_VLinearSum_NrnThreadLD(realtype a, N_Vector x, realtype b, N_Vector y, N_Vector z) {
    apass bpass xpass ypass zpass nrn_multithread_job(vlinearsum);
    mydebug("vlinearsum\n");
    /*pr(z);*/
}

static void* vconst(NrnThread* nt) {
    int i = nt->id;
    N_VConst_Serial(carg, zarg(i));
    return nullptr;
}
void N_VConst_NrnThreadLD(realtype c, N_Vector z) {
    cpass zpass nrn_multithread_job(vconst);
    mydebug("vconst\n");
}

static void* vprod(NrnThread* nt) {
    int i = nt->id;
    N_VProd_Serial(xarg(i), yarg(i), zarg(i));
    return nullptr;
}
void N_VProd_NrnThreadLD(N_Vector x, N_Vector y, N_Vector z) {
    xpass ypass zpass nrn_multithread_job(vprod);
    mydebug("vprod\n");
}

static void* vdiv(NrnThread* nt) {
    int i = nt->id;
    N_VDiv_Serial(xarg(i), yarg(i), zarg(i));
    return nullptr;
}
void N_VDiv_NrnThreadLD(N_Vector x, N_Vector y, N_Vector z) {
    xpass ypass zpass nrn_multithread_job(vdiv);
    mydebug("vdiv\n");
}

static void* vscale(NrnThread* nt) {
    int i = nt->id;
    N_VScale_Serial(carg, xarg(i), zarg(i));
    return nullptr;
}
void N_VScale_NrnThreadLD(realtype c, N_Vector x, N_Vector z) {
    cpass xpass zpass nrn_multithread_job(vscale);
    mydebug("vscale\n");
    /*pr(z);*/
}

static void* vabs(NrnThread* nt) {
    int i = nt->id;
    N_VAbs_Serial(xarg(i), zarg(i));
    return nullptr;
}
void N_VAbs_NrnThreadLD(N_Vector x, N_Vector z) {
    xpass zpass nrn_multithread_job(vabs);
    mydebug("vabs\n");
}

static void* vinv(NrnThread* nt) {
    int i = nt->id;
    N_VInv_Serial(xarg(i), zarg(i));
    return nullptr;
}
void N_VInv_NrnThreadLD(N_Vector x, N_Vector z) {
    xpass zpass nrn_multithread_job(vinv);
    mydebug("vinv\n");
}

static void* vaddconst(NrnThread* nt) {
    int i = nt->id;
    N_VAddConst_Serial(xarg(i), barg, zarg(i));
    return nullptr;
}
void N_VAddConst_NrnThreadLD(N_Vector x, realtype b, N_Vector z) {
    bpass xpass zpass nrn_multithread_job(vaddconst);
    mydebug("vaddconst\n");
}

static void* vdotprod(NrnThread* nt) {
    realtype s;
    int i = nt->id;
    s = N_VDotProd_Serial(xarg(i), yarg(i));
    lockadd(s);
    return nullptr;
}
realtype N_VDotProd_NrnThreadLD(N_Vector x, N_Vector y) {
    retval = ZERO;
    xpass ypass nrn_multithread_job(vdotprod);
    mydebug2("vdotprod %.20g\n", retval);
    return (retval);
}

static void* vmaxnorm(NrnThread* nt) {
    realtype max;
    int i = nt->id;
    max = N_VMaxNorm_Serial(xarg(i));
    lockmax(max);
    return nullptr;
}
realtype N_VMaxNorm_NrnThreadLD(N_Vector x) {
    retval = ZERO;
    xpass nrn_multithread_job(vmaxnorm);
    mydebug2("vmaxnorm %.20g\n", retval);
    return (retval);
}


static realtype vwrmsnorm_help(N_Vector x, N_Vector w) {
    long int i, N;
    realtype *xd, *wd;

    N = NV_LENGTH_S(x);
    xd = NV_DATA_S(x);
    wd = NV_DATA_S(w);

    realtype sum{}, c{};
    for (i = 0; i < N; i++) {
        auto const prodi = (*xd++) * (*wd++);
        auto const y = prodi * prodi - c;
        auto const t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}
static void* vwrmsnorm(NrnThread* nt) {
    int i = nt->id;
    auto const s = vwrmsnorm_help(xarg(i), warg(i));
    // olupton 2023-03-13: this produces results that differ on x86_64 from the long double based
    // calculation at the 1e-16 relative precision level. unfortunately NEURON with cvode amplifies
    // these to larger differences in the precise variable time steps used. however, it should be
    // more consistent across architectures than the long double version.
    lock;
    auto const tmp_y = s - retval_comp;
    auto const tmp_t = retval + tmp_y;
    retval_comp = (tmp_t - retval) - tmp_y;
    retval = tmp_t;
    unlock;
    return nullptr;
}
realtype N_VWrmsNorm_NrnThreadLD(N_Vector x, N_Vector w) {
    long int N;
    N = NV_LENGTH_NT_LD(x);
    retval = retval_comp = 0.0;
    xpass wpass nrn_multithread_job(vwrmsnorm);
    mydebug2("vwrmsnorm %.20g\n", RSqrt(retval / N));
    return (RSqrt(retval / N));
}

static realtype vwrmsnormmask_help(N_Vector x, N_Vector w, N_Vector id) {
    long int i, N;
    realtype sum = ZERO, prodi, *xd, *wd, *idd;

    N = NV_LENGTH_S(x);
    xd = NV_DATA_S(x);
    wd = NV_DATA_S(w);
    idd = NV_DATA_S(id);

    for (i = 0; i < N; i++) {
        if (idd[i] > ZERO) {
            prodi = xd[i] * wd[i];
            sum += prodi * prodi;
        }
    }

    return (sum);
}
static void* vwrmsnormmask(NrnThread* nt) {
    realtype s;
    int i = nt->id;
    s = vwrmsnormmask_help(xarg(i), warg(i), idarg(i));
    lockadd(s);
    return nullptr;
}
realtype N_VWrmsNormMask_NrnThreadLD(N_Vector x, N_Vector w, N_Vector id) {
    long int N;
    N = NV_LENGTH_NT_LD(x);
    retval = ZERO;
    xpass wpass idpass nrn_multithread_job(vwrmsnormmask);
    mydebug2("vwrmsnormmask %.20g\n", RSqrt(retval / N));
    return (RSqrt(retval / N));
}

static void* vmin(NrnThread* nt) {
    realtype min;
    int i = nt->id;
    if (NV_LENGTH_S(xarg(i))) {
        min = N_VMin_Serial(xarg(i));
        lockmin(min);
    }
    return nullptr;
}
realtype N_VMin_NrnThreadLD(N_Vector x) {
    retval = BIG_REAL;
    xpass nrn_multithread_job(vmin);
    mydebug2("vmin %.20g\n", retval);
    return (retval);
}

static realtype N_VWL2Norm_helper(N_Vector x, N_Vector w) {
    long int i, N;
    realtype sum = ZERO, prodi, *xd, *wd;

    N = NV_LENGTH_S(x);
    xd = NV_DATA_S(x);
    wd = NV_DATA_S(w);

    for (i = 0; i < N; i++) {
        prodi = (*xd++) * (*wd++);
        sum += prodi * prodi;
    }

    return sum;
}
static void* vwl2norm(NrnThread* nt) {
    realtype sum;
    int i = nt->id;
    sum = N_VWL2Norm_helper(xarg(i), warg(i));
    lockadd(sum);
    return nullptr;
}
realtype N_VWL2Norm_NrnThreadLD(N_Vector x, N_Vector w) {
    long int N;
    retval = ZERO;
    xpass wpass nrn_multithread_job(vwl2norm);
    N = NV_LENGTH_NT_LD(x);
    mydebug2("vwl2norm %.20g\n", RSqrt(retval));
    return (RSqrt(retval));
}

static void* vl1norm(NrnThread* nt) {
    realtype sum;
    int i = nt->id;
    sum = N_VL1Norm_Serial(xarg(i));
    lockadd(sum);
    return nullptr;
}
realtype N_VL1Norm_NrnThreadLD(N_Vector x) {
    retval = ZERO;
    xpass nrn_multithread_job(vl1norm);
    mydebug2("vl1norm %.20g\n", retval);
    return (retval);
}

static void* v1mask(NrnThread* nt) {
    int i = nt->id;
    N_VOneMask_Serial(xarg(i));
    return nullptr;
}
void N_VOneMask_NrnThreadLD(N_Vector x) {
    xpass nrn_multithread_job(v1mask);
}

static void* vcompare(NrnThread* nt) {
    int i = nt->id;
    N_VCompare_Serial(carg, xarg(i), zarg(i));
    return nullptr;
}
void N_VCompare_NrnThreadLD(realtype c, N_Vector x, N_Vector z) {
    cpass xpass zpass nrn_multithread_job(vcompare);
    mydebug("vcompare\n");
}

static void* vinvtest(NrnThread* nt) {
    booleantype b;
    int i = nt->id;
    b = N_VInvTest_Serial(xarg(i), zarg(i));
    if (!b) {
        lockfalse;
    }
    return nullptr;
}
booleantype N_VInvTest_NrnThreadLD(N_Vector x, N_Vector z) {
    bretval = TRUE;
    xpass zpass nrn_multithread_job(vinvtest);
    mydebug2("vinvtest %d\n", bretval);
    return (bretval);
}

static void* vconstrmask(NrnThread* nt) {
    booleantype b;
    int i = nt->id;
    b = N_VConstrMask_Serial(yarg(i), xarg(i), zarg(i));
    if (!b) {
        lockfalse;
    }
    return nullptr;
}
booleantype N_VConstrMask_NrnThreadLD(N_Vector y, N_Vector x, N_Vector z) {
    bretval = TRUE;
    ypass xpass zpass nrn_multithread_job(vconstrmask);
    mydebug2("vconstrmask %d\n", bretval);
    return (bretval);
}

static void* vminquotient(NrnThread* nt) {
    realtype min;
    int i = nt->id;
    min = N_VMinQuotient_Serial(xarg(i), yarg(i));
    lockmin(min);
    return nullptr;
}
realtype N_VMinQuotient_NrnThreadLD(N_Vector x, N_Vector y) /* num, denom */
{
    retval = BIG_REAL;
    xpass ypass nrn_multithread_job(vconstrmask);
    mydebug2("vminquotient %.20g\n", retval);
    return (retval);
}

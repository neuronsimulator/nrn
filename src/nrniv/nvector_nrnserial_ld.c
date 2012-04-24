/*
 * -----------------------------------------------------------------
 * $Revision: 2212 $
 * $Date: 2008-09-08 16:32:26 +0200 (Mon, 08 Sep 2008) $
 * ----------------------------------------------------------------- 
 * Programmer(s): Scott D. Cohen, Alan C. Hindmarsh, Radu Serban,
 *                and Aaron Collier @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California.
 * Produced at the Lawrence Livermore National Laboratory.
 * All rights reserved.
 * For details, see sundials/shared/LICENSE.
 * -----------------------------------------------------------------
 * This is the implementation file for a serial implementation
 * of the NVECTOR package.
 * -----------------------------------------------------------------
 */

#define USELONGDOUBLE 1

#include <../../nrnconf.h>
#include <hocassrt.h>
#if HAVE_POSIX_MEMALIGN
#define HAVE_MEMALIGN 1
#endif
#if HAVE_MEMALIGN
#define _XOPEN_SOURCE 600
#endif

#include <stdio.h>
#include <stdlib.h>

#include "nvector_nrnserial_ld.h"
#include "shared/sundialsmath.h"
#include "shared/sundialstypes.h"

#define ZERO   RCONST(0.0)
#define HALF   RCONST(0.5)
#define ONE    RCONST(1.0)
#define ONEPT5 RCONST(1.5)

#if USELONGDOUBLE
#define ldrealtype long double
#else
#define ldrealtype realtype
#endif

/* Private function prototypes */
/* z=x */
static void VCopy_NrnSerialLD(N_Vector x, N_Vector z);
/* z=x+y */
static void VSum_NrnSerialLD(N_Vector x, N_Vector y, N_Vector z);
/* z=x-y */
static void VDiff_NrnSerialLD(N_Vector x, N_Vector y, N_Vector z);
/* z=-x */
static void VNeg_NrnSerialLD(N_Vector x, N_Vector z);
/* z=c(x+y) */
static void VScaleSum_NrnSerialLD(realtype c, N_Vector x, N_Vector y, N_Vector z);
/* z=c(x-y) */
static void VScaleDiff_NrnSerialLD(realtype c, N_Vector x, N_Vector y, N_Vector z); 
/* z=ax+y */
static void VLin1_NrnSerialLD(realtype a, N_Vector x, N_Vector y, N_Vector z);
/* z=ax-y */
static void VLin2_NrnSerialLD(realtype a, N_Vector x, N_Vector y, N_Vector z);
/* y <- ax+y */
static void Vaxpy_NrnSerialLD(realtype a, N_Vector x, N_Vector y);
/* x <- ax */
static void VScaleBy_NrnSerialLD(realtype a, N_Vector x);

/*
 * -----------------------------------------------------------------
 * exported functions
 * -----------------------------------------------------------------
 */

/* ----------------------------------------------------------------------------
 * Function to create a new empty serial vector 
 */

N_Vector N_VNewEmpty_NrnSerialLD(long int length)
{
  N_Vector v;
  N_Vector_Ops ops;
  N_VectorContent_NrnSerialLD content;

  /* Create vector */
  v = (N_Vector) malloc(sizeof *v);
  if (v == NULL) return(NULL);
  
  /* Create vector operation structure */
  ops = (N_Vector_Ops) malloc(sizeof(struct _generic_N_Vector_Ops));
  if (ops == NULL) {free(v);return(NULL);}

  ops->nvclone           = N_VClone_NrnSerialLD;
  ops->nvdestroy         = N_VDestroy_NrnSerialLD;
  ops->nvspace           = N_VSpace_NrnSerialLD;
  ops->nvgetarraypointer = N_VGetArrayPointer_NrnSerialLD;
  ops->nvsetarraypointer = N_VSetArrayPointer_NrnSerialLD;
  ops->nvlinearsum       = N_VLinearSum_NrnSerialLD;
  ops->nvconst           = N_VConst_NrnSerialLD;
  ops->nvprod            = N_VProd_NrnSerialLD;
  ops->nvdiv             = N_VDiv_NrnSerialLD;
  ops->nvscale           = N_VScale_NrnSerialLD;
  ops->nvabs             = N_VAbs_NrnSerialLD;
  ops->nvinv             = N_VInv_NrnSerialLD;
  ops->nvaddconst        = N_VAddConst_NrnSerialLD;
  ops->nvdotprod         = N_VDotProd_NrnSerialLD;
  ops->nvmaxnorm         = N_VMaxNorm_NrnSerialLD;
  ops->nvwrmsnormmask    = N_VWrmsNormMask_NrnSerialLD;
  ops->nvwrmsnorm        = N_VWrmsNorm_NrnSerialLD;
  ops->nvmin             = N_VMin_NrnSerialLD;
  ops->nvwl2norm         = N_VWL2Norm_NrnSerialLD;
  ops->nvl1norm          = N_VL1Norm_NrnSerialLD;
  ops->nvcompare         = N_VCompare_NrnSerialLD;
  ops->nvinvtest         = N_VInvTest_NrnSerialLD;
  ops->nvconstrmask      = N_VConstrMask_NrnSerialLD;
  ops->nvminquotient     = N_VMinQuotient_NrnSerialLD;

  /* Create content */
  content = (N_VectorContent_NrnSerialLD) malloc(sizeof(struct _N_VectorContent_NrnSerialLD));
  if (content == NULL) {free(ops);free(v);return(NULL);}

  content->length = length;
  content->own_data = FALSE;
  content->data = NULL;

  /* Attach content and ops */
  v->content = content;
  v->ops = ops;

  return(v);
}

/* ----------------------------------------------------------------------------
 * Function to create a new serial vector 
 */

N_Vector N_VNew_NrnSerialLD(long int length)
{
  N_Vector v;
  realtype *data;

  v = N_VNewEmpty_NrnSerialLD(length);
  if (v == NULL) return(NULL);

  /* Create data */
  if (length > 0) {

    /* Allocate memory */
#if HAVE_MEMALIGN
    assert(posix_memalign((void**)&data, 64, length*sizeof(realtype)) == 0);
#else
    data = (realtype *) malloc(length * sizeof(realtype));
#endif
    if(data == NULL) {N_VDestroy_NrnSerialLD(v);return(NULL);}

    /* Attach data */
    NV_OWN_DATA_S_LD(v) = TRUE;
    NV_DATA_S_LD(v) = data;

  }

  return(v);
}

/* ----------------------------------------------------------------------------
 * Function to clone from a template a new vector with empty (NULL) data array
 */

N_Vector N_VCloneEmpty_NrnSerialLD(N_Vector w)
{
  N_Vector v;
  N_Vector_Ops ops;
  N_VectorContent_NrnSerialLD content;

  if (w == NULL) return(NULL);

  /* Create vector */
  v = (N_Vector) malloc(sizeof *v);
  if (v == NULL) return(NULL);

  /* Create vector operation structure */
  ops = (N_Vector_Ops) malloc(sizeof(struct _generic_N_Vector_Ops));
  if (ops == NULL) {free(v);return(NULL);}
  
  ops->nvclone           = w->ops->nvclone;
  ops->nvdestroy         = w->ops->nvdestroy;
  ops->nvspace           = w->ops->nvspace;
  ops->nvgetarraypointer = w->ops->nvgetarraypointer;
  ops->nvsetarraypointer = w->ops->nvsetarraypointer;
  ops->nvlinearsum       = w->ops->nvlinearsum;
  ops->nvconst           = w->ops->nvconst;  
  ops->nvprod            = w->ops->nvprod;   
  ops->nvdiv             = w->ops->nvdiv;
  ops->nvscale           = w->ops->nvscale; 
  ops->nvabs             = w->ops->nvabs;
  ops->nvinv             = w->ops->nvinv;
  ops->nvaddconst        = w->ops->nvaddconst;
  ops->nvdotprod         = w->ops->nvdotprod;
  ops->nvmaxnorm         = w->ops->nvmaxnorm;
  ops->nvwrmsnormmask    = w->ops->nvwrmsnormmask;
  ops->nvwrmsnorm        = w->ops->nvwrmsnorm;
  ops->nvmin             = w->ops->nvmin;
  ops->nvwl2norm         = w->ops->nvwl2norm;
  ops->nvl1norm          = w->ops->nvl1norm;
  ops->nvcompare         = w->ops->nvcompare;    
  ops->nvinvtest         = w->ops->nvinvtest;
  ops->nvconstrmask      = w->ops->nvconstrmask;
  ops->nvminquotient     = w->ops->nvminquotient;

  /* Create content */
  content = (N_VectorContent_NrnSerialLD) malloc(sizeof(struct _N_VectorContent_NrnSerialLD));
  if (content == NULL) {free(ops);free(v);return(NULL);}

  content->length = NV_LENGTH_S_LD(w);
  content->own_data = FALSE;
  content->data = NULL;

  /* Attach content and ops */
  v->content = content;
  v->ops = ops;

  return(v);
}

/* ----------------------------------------------------------------------------
 * Function to create a serial N_Vector with user data component 
 */

N_Vector N_VMake_NrnSerialLD(long int length, realtype *v_data)
{
  N_Vector v;

  v = N_VNewEmpty_NrnSerialLD(length);
  if (v == NULL) return(NULL);

  if (length > 0) {
    /* Attach data */
    NV_OWN_DATA_S_LD(v) = FALSE;
    NV_DATA_S_LD(v) = v_data;
  }

  return(v);
}

/* ----------------------------------------------------------------------------
 * Function to create an array of new serial vectors. 
 */

N_Vector *N_VNewVectorArray_NrnSerialLD(int count, long int length)
{
  N_Vector *vs;
  int j;

  if (count <= 0) return(NULL);

  vs = (N_Vector *) malloc(count * sizeof(N_Vector));
  if(vs == NULL) return(NULL);

  for (j=0; j<count; j++) {
    vs[j] = N_VNew_NrnSerialLD(length);
    if (vs[j] == NULL) {
      N_VDestroyVectorArray_NrnSerialLD(vs, j-1);
      return(NULL);
    }
  }

  return(vs);
}

/* ----------------------------------------------------------------------------
 * Function to create an array of new serial vectors with NULL data array. 
 */

N_Vector *N_VNewVectorArrayEmpty_NrnSerialLD(int count, long int length)
{
  N_Vector *vs;
  int j;

  if (count <= 0) return(NULL);

  vs = (N_Vector *) malloc(count * sizeof(N_Vector));
  if(vs == NULL) return(NULL);

  for (j=0; j<count; j++) {
    vs[j] = N_VNewEmpty_NrnSerialLD(length);
    if (vs[j] == NULL) {
      N_VDestroyVectorArray_NrnSerialLD(vs, j-1);
      return(NULL);
    }
  }

  return(vs);
}

/* ----------------------------------------------------------------------------
 * Function to free an array created with N_VNewVectorArray_NrnSerialLD
 */

void N_VDestroyVectorArray_NrnSerialLD(N_Vector *vs, int count)
{
  int j;

  for (j = 0; j < count; j++) N_VDestroy_NrnSerialLD(vs[j]);

  free(vs);
}

/* ----------------------------------------------------------------------------
 * Function to print the a serial vector 
 */
 
void N_VPrint_NrnSerialLD(N_Vector x)
{
  long int i, N;
  realtype *xd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);

  for (i=0; i < N; i++) {
#if defined(SUNDIALS_EXTENDED_PRECISION)
    printf("%11.8Lg\n", *xd++);
#elif defined(SUNDIALS_DOUBLE_PRECISION)
    printf("%11.8lg\n", *xd++);
#else
    printf("%11.8g\n", *xd++);
#endif
  }
  printf("\n");
}

/*
 * -----------------------------------------------------------------
 * implementation of vector operations
 * -----------------------------------------------------------------
 */

N_Vector N_VClone_NrnSerialLD(N_Vector w)
{
  N_Vector v;
  realtype *data;
  long int length;

  v = N_VCloneEmpty_NrnSerialLD(w);
  if (v == NULL) return(NULL);

  length = NV_LENGTH_S_LD(w);

  /* Create data */
  if (length > 0) {

    /* Allocate memory */
#if HAVE_MEMALIGN
    assert(posix_memalign((void**)&data, 64, length*sizeof(realtype)) == 0);
#else
    data = (realtype *) malloc(length * sizeof(realtype));
#endif
    if(data == NULL) {N_VDestroy_NrnSerialLD(v);return(NULL);}

    /* Attach data */
    NV_OWN_DATA_S_LD(v) = TRUE;
    NV_DATA_S_LD(v) = data;

  }

  return(v);
}

void N_VDestroy_NrnSerialLD(N_Vector v)
{
  if (NV_OWN_DATA_S_LD(v) == TRUE)
    free(NV_DATA_S_LD(v));
  free(v->content);
  free(v->ops);
  free(v);
}

void N_VSpace_NrnSerialLD(N_Vector v, long int *lrw, long int *liw)
{
  *lrw = NV_LENGTH_S_LD(v);
  *liw = 1;
}

realtype *N_VGetArrayPointer_NrnSerialLD(N_Vector v)
{
  realtype *v_data;

  v_data = NV_DATA_S_LD(v);

  return(v_data);
}

void N_VSetArrayPointer_NrnSerialLD(realtype *v_data, N_Vector v)
{
  if (NV_LENGTH_S_LD(v) > 0) NV_DATA_S_LD(v) = v_data;
}

void N_VLinearSum_NrnSerialLD(realtype a, N_Vector x, realtype b, N_Vector y, N_Vector z)
{
  long int i, N;
  realtype c, *xd, *yd, *zd;
  N_Vector v1, v2;
  booleantype test;

  if ((b == ONE) && (z == y)) {    /* BLAS usage: axpy y <- ax+y */
    Vaxpy_NrnSerialLD(a,x,y);
    return;
  }

  if ((a == ONE) && (z == x)) {    /* BLAS usage: axpy x <- by+x */
    Vaxpy_NrnSerialLD(b,y,x);
    return;
  }

  /* Case: a == b == 1.0 */

  if ((a == ONE) && (b == ONE)) {
    VSum_NrnSerialLD(x, y, z);
    return;
  }

  /* Cases: (1) a == 1.0, b = -1.0, (2) a == -1.0, b == 1.0 */

  if ((test = ((a == ONE) && (b == -ONE))) || ((a == -ONE) && (b == ONE))) {
    v1 = test ? y : x;
    v2 = test ? x : y;
    VDiff_NrnSerialLD(v2, v1, z);
    return;
  }

  /* Cases: (1) a == 1.0, b == other or 0.0, (2) a == other or 0.0, b == 1.0 */
  /* if a or b is 0.0, then user should have called N_VScale */

  if ((test = (a == ONE)) || (b == ONE)) {
    c = test ? b : a;
    v1 = test ? y : x;
    v2 = test ? x : y;
    VLin1_NrnSerialLD(c, v1, v2, z);
    return;
  }

  /* Cases: (1) a == -1.0, b != 1.0, (2) a != 1.0, b == -1.0 */

  if ((test = (a == -ONE)) || (b == -ONE)) {
    c = test ? b : a;
    v1 = test ? y : x;
    v2 = test ? x : y;
    VLin2_NrnSerialLD(c, v1, v2, z);
    return;
  }

  /* Case: a == b */
  /* catches case both a and b are 0.0 - user should have called N_VConst */

  if (a == b) {
    VScaleSum_NrnSerialLD(a, x, y, z);
    return;
  }

  /* Case: a == -b */

  if (a == -b) {
    VScaleDiff_NrnSerialLD(a, x, y, z);
    return;
  }

  /* Do all cases not handled above:
     (1) a == other, b == 0.0 - user should have called N_VScale
     (2) a == 0.0, b == other - user should have called N_VScale
     (3) a,b == other, a !=b, a != -b */
  
  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++) 
    *zd++ = a * (*xd++) + b * (*yd++);
}

void N_VConst_NrnSerialLD(realtype c, N_Vector z)
{
  long int i, N;
  realtype *zd;

  N  = NV_LENGTH_S_LD(z);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++) 
    *zd++ = c;
}

void N_VProd_NrnSerialLD(N_Vector x, N_Vector y, N_Vector z)
{
  long int i, N;
  realtype *xd, *yd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++)
    *zd++ = (*xd++) * (*yd++);
}

void N_VDiv_NrnSerialLD(N_Vector x, N_Vector y, N_Vector z)
{
  long int i, N;
  realtype *xd, *yd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++)
    *zd++ = (*xd++) / (*yd++);
}

void N_VScale_NrnSerialLD(realtype c, N_Vector x, N_Vector z)
{
  long int i, N;
  realtype *xd, *zd;

  if (z == x) {       /* BLAS usage: scale x <- cx */
    VScaleBy_NrnSerialLD(c, x);
    return;
  }

  if (c == ONE) {
    VCopy_NrnSerialLD(x, z);
  } else if (c == -ONE) {
    VNeg_NrnSerialLD(x, z);
  } else {
    N  = NV_LENGTH_S_LD(x);
    xd = NV_DATA_S_LD(x);
    zd = NV_DATA_S_LD(z);
    for (i=0; i < N; i++) 
      *zd++ = c * (*xd++);
  }
}

void N_VAbs_NrnSerialLD(N_Vector x, N_Vector z)
{
  long int i, N;
  realtype *xd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++, xd++, zd++)
    *zd = ABS(*xd);
}

void N_VInv_NrnSerialLD(N_Vector x, N_Vector z)
{
  long int i, N;
  realtype *xd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++)
    *zd++ = ONE / (*xd++);
}

void N_VAddConst_NrnSerialLD(N_Vector x, realtype b, N_Vector z)
{
  long int i, N;
  realtype *xd, *zd;
  
  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++) 
    *zd++ = (*xd++) + b; 
}

realtype N_VDotProd_NrnSerialLD(N_Vector x, N_Vector y)
{
  long int i, N;
  realtype sum = ZERO, *xd, *yd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);

  for (i=0; i < N; i++)
    sum += (*xd++) * (*yd++);
  
  return(sum);
}

realtype N_VMaxNorm_NrnSerialLD(N_Vector x)
{
  long int i, N;
  realtype max = ZERO, *xd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);

  for (i=0; i < N; i++, xd++) {
    if (ABS(*xd) > max) max = ABS(*xd);
  }
   
  return(max);
}

realtype N_VWrmsNorm_NrnSerialLD(N_Vector x, N_Vector w)
{
  long int i, N;
  realtype prodi, *xd, *wd;
  ldrealtype sum = ZERO;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  wd = NV_DATA_S_LD(w);

  for (i=0; i < N; i++) {
    prodi = (*xd++) * (*wd++);
    sum += prodi * prodi;
  }

  return(RSqrt((realtype)sum / N));
}

realtype N_VWrmsNormMask_NrnSerialLD(N_Vector x, N_Vector w, N_Vector id)
{
  long int i, N;
  realtype prodi, *xd, *wd, *idd;
  ldrealtype sum = ZERO;

  N  = NV_LENGTH_S_LD(x);
  xd  = NV_DATA_S_LD(x);
  wd  = NV_DATA_S_LD(w);
  idd = NV_DATA_S_LD(id);

  for (i=0; i < N; i++) {
    if (idd[i] > ZERO) {
      prodi = xd[i] * wd[i];
      sum += prodi * prodi;
    }
  }

  return(RSqrt((realtype)sum / N));
}

realtype N_VMin_NrnSerialLD(N_Vector x)
{
  long int i, N;
  realtype min, *xd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);

  min = xd[0];

  xd++;
  for (i=1; i < N; i++, xd++) {
    if ((*xd) < min) min = *xd;
  }

  return(min);
}

realtype N_VWL2Norm_NrnSerialLD(N_Vector x, N_Vector w)
{
  long int i, N;
  realtype prodi, *xd, *wd;
  ldrealtype sum = ZERO;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  wd = NV_DATA_S_LD(w);

  for (i=0; i < N; i++) {
    prodi = (*xd++) * (*wd++);
    sum += prodi * prodi;
  }

  return(RSqrt((realtype)sum));
}

realtype N_VL1Norm_NrnSerialLD(N_Vector x)
{
  long int i, N;
  realtype *xd;
  ldrealtype sum = ZERO;
  
  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  
  for (i=0; i<N; i++)  
    sum += ABS(xd[i]);

  return((realtype)sum);
}

void N_VOneMask_NrnSerialLD(N_Vector x)
{
  long int i, N;
  realtype *xd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);

  for (i=0; i<N; i++,xd++) {
    if (*xd != ZERO) *xd = ONE;
  }
}

void N_VCompare_NrnSerialLD(realtype c, N_Vector x, N_Vector z)
{
  long int i, N;
  realtype *xd, *zd;
  
  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++, xd++, zd++) {
    *zd = (ABS(*xd) >= c) ? ONE : ZERO;
  }
}

booleantype N_VInvTest_NrnSerialLD(N_Vector x, N_Vector z)
{
  long int i, N;
  realtype *xd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++) {
    if (*xd == ZERO) return(FALSE);
    *zd++ = ONE / (*xd++);
  }

  return(TRUE);
}

booleantype N_VConstrMask_NrnSerialLD(N_Vector c, N_Vector x, N_Vector m)
{
  long int i, N;
  booleantype test;
  realtype *cd, *xd, *md;
  
  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  cd = NV_DATA_S_LD(c);
  md = NV_DATA_S_LD(m);

  test = TRUE;

  for (i=0; i<N; i++, cd++, xd++, md++) {
    *md = ZERO;
    if (*cd == ZERO) continue;
    if (*cd > ONEPT5 || (*cd) < -ONEPT5) {
      if ( (*xd)*(*cd) <= ZERO) {test = FALSE; *md = ONE; }
      continue;
    }
    if ( (*cd) > HALF || (*cd) < -HALF) {
      if ( (*xd)*(*cd) < ZERO ) {test = FALSE; *md = ONE; }
    }
  }
  return(test);
}

realtype N_VMinQuotient_NrnSerialLD(N_Vector num, N_Vector denom)
{
  booleantype notEvenOnce;
  long int i, N;
  realtype *nd, *dd, min;

  N  = NV_LENGTH_S_LD(num);
  nd = NV_DATA_S_LD(num);
  dd = NV_DATA_S_LD(denom);

  notEvenOnce = TRUE;

  for (i = 0; i < N; i++, nd++, dd++) {
    if (*dd == ZERO) continue;
    else {
      if (notEvenOnce) {
        min = *nd / *dd ;
        notEvenOnce = FALSE;
      }
      else min = MIN(min, (*nd) / (*dd));
    }
  }

  if (notEvenOnce || (N == 0)) min = BIG_REAL;
  
  return(min);
}

/*
 * -----------------------------------------------------------------
 * private functions
 * -----------------------------------------------------------------
 */

static void VCopy_NrnSerialLD(N_Vector x, N_Vector z)
{
  long int i, N;
  realtype *xd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++)
    *zd++ = *xd++; 
}

static void VSum_NrnSerialLD(N_Vector x, N_Vector y, N_Vector z)
{
  long int i, N;
  realtype *xd, *yd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++)
    *zd++ = (*xd++) + (*yd++);
}

static void VDiff_NrnSerialLD(N_Vector x, N_Vector y, N_Vector z)
{
  long int i, N;
  realtype *xd, *yd, *zd;
 
  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++)
    *zd++ = (*xd++) - (*yd++);
}

static void VNeg_NrnSerialLD(N_Vector x, N_Vector z)
{
  long int i, N;
  realtype *xd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  zd = NV_DATA_S_LD(z);
  
  for (i=0; i < N; i++)
    *zd++ = -(*xd++);
}

static void VScaleSum_NrnSerialLD(realtype c, N_Vector x, N_Vector y, N_Vector z)
{
  long int i, N;
  realtype *xd, *yd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++)
    *zd++ = c * ((*xd++) + (*yd++));
}

static void VScaleDiff_NrnSerialLD(realtype c, N_Vector x, N_Vector y, N_Vector z)
{
  long int i, N;
  realtype *xd, *yd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++)
    *zd++ = c * ((*xd++) - (*yd++));
}

static void VLin1_NrnSerialLD(realtype a, N_Vector x, N_Vector y, N_Vector z)
{
  long int i, N;
  realtype *xd, *yd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++)
    *zd++ = a * (*xd++) + (*yd++);
}

static void VLin2_NrnSerialLD(realtype a, N_Vector x, N_Vector y, N_Vector z)
{
  long int i, N;
  realtype *xd, *yd, *zd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);
  zd = NV_DATA_S_LD(z);

  for (i=0; i < N; i++)
    *zd++ = a * (*xd++) - (*yd++);
}

static void Vaxpy_NrnSerialLD(realtype a, N_Vector x, N_Vector y)
{
  long int i, N;
  realtype *xd, *yd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);
  yd = NV_DATA_S_LD(y);

  if (a == ONE) {
    for (i=0; i < N; i++)
      *yd++ += (*xd++);
    return;
  }

  if (a == -ONE) {
    for (i=0; i < N; i++)
      *yd++ -= (*xd++);
    return;
  }    

  for (i=0; i < N; i++)
    *yd++ += a * (*xd++);
}

static void VScaleBy_NrnSerialLD(realtype a, N_Vector x)
{
  long int i, N;
  realtype *xd;

  N  = NV_LENGTH_S_LD(x);
  xd = NV_DATA_S_LD(x);

  for (i=0; i < N; i++)
    *xd++ *= a;
}

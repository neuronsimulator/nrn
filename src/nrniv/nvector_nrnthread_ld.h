/*
* N_Vector_NrnThreadLD derived from N_Vector_Serial SunDials version
* by replacing every occurrence of nrnthread with nrnthread in the various
* cases and then modifying the relevant prototypes.
* We only re-implement the ones that are used by cvodes and ida
*/
/*
Macros changed with
sed 's/NV_\([A-Za-z_]*\)_NT/NV_\1_NT_LD/g' nvector_nrnthread_ld.c > temp
mv temp nvector_nrnthread_ld.c
sed 's/NV_\([A-Za-z_]*\)_NT/NV_\1_NT_LD/g' nvector_nrnthread_ld.h >temp
mv temp nvector_nrnthread_ld.h
*/
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
 * This is the header file for the nrnthread implementation of the
 * NVECTOR module.
 *
 * Part I contains declarations specific to the nrnthread
 * implementation of the supplied NVECTOR module.
 *
 * Part II defines accessor macros that allow the user to
 * efficiently use the type N_Vector without making explicit
 * references to the underlying data structure.
 *
 * Part III contains the prototype for the constructor N_VNew_NrnThreadLD
 * as well as implementation-specific prototypes for various useful
 * vector operations.
 *
 * Notes:
 *
 *   - The definition of the generic N_Vector structure can be found
 *     in the header file shared/include/nvector.h.
 *
 *   - The definition of the type realtype can be found in the
 *     header file shared/include/sundialstypes.h, and it may be
 *     changed (at the configuration stage) according to the user's
 *     needs. The sundialstypes.h file also contains the definition
 *     for the type booleantype.
 *
 *   - N_Vector arguments to arithmetic vector operations need not
 *     be distinct. For example, the following call:
 *
 *       N_VLinearSum_NrnThreadLD(a,x,b,y,y);
 *
 *     (which stores the result of the operation a*x+b*y in y)
 *     is legal.
 * -----------------------------------------------------------------
 */

#ifndef _NVECTOR_NRNTHREAD_LD_H
#define _NVECTOR_NRNTHREAD_LD_H

#ifdef __cplusplus  /* wrapper to enable C++ usage */
extern "C" {
#endif

#include "nvector.h"
#include "sundialstypes.h"
extern void N_VOneMask_Serial(N_Vector x);

/*
 * -----------------------------------------------------------------
 * PART I: NRNTHREAD implementation of N_Vector
 * -----------------------------------------------------------------
 */

/* nrnthread implementation of the N_Vector 'content' structure
   contains the length of the vector, nthread, and a pointer to
   nthread N_Vector (serial)
*/

struct _N_VectorContent_NrnThreadLD {
  long int length;
  int nt; /* number of threads */
  booleantype own_data;
  N_Vector *data; /* nt of them (N_Vector_Serial) */
};

typedef struct _N_VectorContent_NrnThreadLD *N_VectorContent_NrnThreadLD;

/* Note: documentation below may not be completely transformed from the
   N_Vector_Serial case
*/

/*
 * -----------------------------------------------------------------
 * PART II: macros NV_CONTENT_S, NV_DATA_S, NV_OWN_DATA_S,
 *          NV_LENGTH_S, and NV_Ith_S
 * -----------------------------------------------------------------
 * In the descriptions below, the following user declarations
 * are assumed:
 *
 * N_Vector v;
 * long int i;
 *
 * (1) NV_CONTENT_NT_LD
 *
 *     This routines gives access to the contents of the nrnthread
 *     vector N_Vector.
 *
 *     The assignment v_cont = NV_CONTENT_NT_LD(v) sets v_cont to be
 *     a pointer to the nrnthread N_Vector content structure.
 *
 * (2) NV_SUBVEC_NT_LD and NV_LENGTH_NT_LD
 *
 *     These routines give access to the individual parts of
 *     the content structure of a nrnthread N_Vector.
 *
 *     The assignment v_data = NV_SUBVEC_NT_LD(v, i) sets v_data to be
 *     a pointer to the ith N_Vector of v. The assignment
 *     NV_SUBVEC_NT_LD(v, i) = data_V sets the ith component N_Vector of v to
 *     be data_v by storing the pointer data_v.
 *
 *     The assignment v_llen = NV_SIZE_NT_LD(v,i) sets v_llen to be
 *     the length of the ith component of v. The call NV_LENGTH_NT_LD(v) = len_v sets
 *     the length of v to be len_v.
 *
 * (3) NV_Ith_NT_LD
 *
 *     In the following description, the components of an
 *     N_Vector are numbered 0..n-1, where n is the length of v.
 *
 *     The assignment r = NV_Ith_S(v,i) sets r to be the value of
 *     the ith component of v. The assignment NV_Ith_S(v,i) = r
 *     sets the value of the ith component of v to be r.
 *
 * Note: When looping over the components of an N_Vector v, it is
 * more efficient to first obtain the component array via
 * v_data = NV_DATA_S(v) and then access v_data[i] within the
 * loop than it is to use NV_Ith_S(v,i) within the loop.
 * -----------------------------------------------------------------
 */

#define NV_CONTENT_NT_LD(v)  ( (N_VectorContent_NrnThreadLD)(v->content) )

#define NV_LENGTH_NT_LD(v)   ( NV_CONTENT_NT_LD(v)->length )

#define NV_NT_NT_LD(v)   ( NV_CONTENT_NT_LD(v)->nt )

#define NV_OWN_DATA_NT_LD(v) ( NV_CONTENT_NT_LD(v)->own_data )

#define NV_DATA_NT_LD(v)     ( NV_CONTENT_NT_LD(v)->data )

#define NV_SUBVEC_NT_LD(v, i)     ( NV_CONTENT_NT_LD(v)->data[i] )

#define NV_Ith_NT_LD(v,i)    ( NV_DATA_NT_LD(v)[i] ) /* wrong but not needed */

/*
 * -----------------------------------------------------------------
 * PART III: functions exported by nvector_nrnthread
 * 
 * CONSTRUCTORS:
 *    N_VNew_NrnThreadLD
 *    N_VNewEmpty_NrnThreadLD
 *    N_VClone_NrnThreadLD
 *    N_VCloneEmpty_NrnThreadLD
 *    N_VMake_NrnThreadLD
 *    N_VNewVectorArray_NrnThreadLD
 *    N_VNewVectorArrayEmpty_NrnThreadLD
 * DESTRUCTORS:
 *    N_VDestroy_NrnThreadLD
 *    N_VDestroyVectorArray_NrnThreadLD
 * -----------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------
 * Function : N_VNew_NrnThreadLD
 * -----------------------------------------------------------------
 * This function creates and allocates memory for a nrnthread vector.
 * -----------------------------------------------------------------
 */

N_Vector N_VNew_NrnThreadLD(long int vec_length, int nthread, long int* sizes );

/*
 * -----------------------------------------------------------------
 * Function : N_VNewEmpty_NrnThreadLD
 * -----------------------------------------------------------------
 * This function creates a new nrnthread N_Vector with an empty (NULL)
 * data array.
 * -----------------------------------------------------------------
 */

N_Vector N_VNewEmpty_NrnThreadLD(long int vec_length, int nthread, long int* sizes);

/*
 * -----------------------------------------------------------------
 * Function : N_VCloneEmpty_NrnThreadLD
 * -----------------------------------------------------------------
 * This function creates a new nrnthread N_Vector with an empty (NULL)
 * data array.
 * -----------------------------------------------------------------
 */

N_Vector N_VCloneEmpty_NrnThreadLD(N_Vector w);

/*
 * -----------------------------------------------------------------
 * Function : N_VMake_NrnThreadLD
 * -----------------------------------------------------------------
 * This function creates and allocates memory for a nrnthread vector
 * with a user-supplied data array.
 * -----------------------------------------------------------------
 */

/*not implemented*/
N_Vector N_VMake_NrnThreadLD(long int vec_length, realtype *v_data);

/*
 * -----------------------------------------------------------------
 * Function : N_VNewVectorArray_NrnThreadLD
 * -----------------------------------------------------------------
 * This function creates an array of 'count' nrnthread vectors. This
 * array of N_Vectors can be freed using N_VDestroyVectorArray
 * (defined by the generic NVECTOR module).
 * -----------------------------------------------------------------
 */

N_Vector *N_VNewVectorArray_NrnThreadLD(int count, long int vec_length,
	int nthread, long int* sizes);

/*
 * -----------------------------------------------------------------
 * Function : N_VNewVectorArrayEmpty_NrnThreadLD
 * -----------------------------------------------------------------
 * This function creates an array of 'count' nrnthread vectors each
 * with an empty (NULL) data array.
 * -----------------------------------------------------------------
 */

N_Vector *N_VNewVectorArrayEmpty_NrnThreadLD(int count, long int vec_length,
	int nthread, long int* sizes);

/*
 * -----------------------------------------------------------------
 * Function : N_VDestroyVectorArray_NrnThreadLD
 * -----------------------------------------------------------------
 * This function frees an array of N_Vector created with 
 * N_VNewVectorArray_NrnThreadLD.
 * -----------------------------------------------------------------
 */

void N_VDestroyVectorArray_NrnThreadLD(N_Vector *vs, int count);

/*
 * -----------------------------------------------------------------
 * Function : N_VPrint_NrnThreadLD
 * -----------------------------------------------------------------
 * This function prints the content of a nrnthread vector to stdout.
 * -----------------------------------------------------------------
 */

void N_VPrint_NrnThreadLD(N_Vector v);

/*
 * -----------------------------------------------------------------
 * nrnthread implementations of various useful vector operations
 * -----------------------------------------------------------------
 */

N_Vector N_VClone_NrnThreadLD(N_Vector w);
void N_VDestroy_NrnThreadLD(N_Vector v);
void N_VSpace_NrnThreadLD(N_Vector v, long int *lrw, long int *liw);
realtype *N_VGetArrayPointer_NrnThreadLD(N_Vector v);
void N_VSetArrayPointer_NrnThreadLD(realtype *v_data, N_Vector v);
void N_VLinearSum_NrnThreadLD(realtype a, N_Vector x, realtype b, N_Vector y, N_Vector z);
void N_VConst_NrnThreadLD(realtype c, N_Vector z);
void N_VProd_NrnThreadLD(N_Vector x, N_Vector y, N_Vector z);
void N_VDiv_NrnThreadLD(N_Vector x, N_Vector y, N_Vector z);
void N_VScale_NrnThreadLD(realtype c, N_Vector x, N_Vector z);
void N_VAbs_NrnThreadLD(N_Vector x, N_Vector z);
void N_VInv_NrnThreadLD(N_Vector x, N_Vector z);
void N_VAddConst_NrnThreadLD(N_Vector x, realtype b, N_Vector z);
realtype N_VDotProd_NrnThreadLD(N_Vector x, N_Vector y);
realtype N_VMaxNorm_NrnThreadLD(N_Vector x);
realtype N_VWrmsNorm_NrnThreadLD(N_Vector x, N_Vector w);
realtype N_VWrmsNormMask_NrnThreadLD(N_Vector x, N_Vector w, N_Vector id);
realtype N_VMin_NrnThreadLD(N_Vector x);
realtype N_VWL2Norm_NrnThreadLD(N_Vector x, N_Vector w);
realtype N_VL1Norm_NrnThreadLD(N_Vector x);
void N_VCompare_NrnThreadLD(realtype c, N_Vector x, N_Vector z);
booleantype N_VInvTest_NrnThreadLD(N_Vector x, N_Vector z);
booleantype N_VConstrMask_NrnThreadLD(N_Vector c, N_Vector x, N_Vector m);
realtype N_VMinQuotient_NrnThreadLD(N_Vector num, N_Vector denom);

#ifdef __cplusplus
}
#endif

#endif

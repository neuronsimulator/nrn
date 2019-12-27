/*
* N_Vector_NrnSerialLD derived from N_Vector_Serial Sundials version
* by replaceing every occurrence of Serial with NrnSerialLD and then
* modifying relevant method implementations to allow long double
* accumulation.
*/
/*
Macros changed with
sed 's/NV_\([A-Za-z_]*\)_S/NV_\1_S_LD/g' nvector_nrnserial_ld.h >temp
mv temp nvector_nrnserial_ld.h
sed 's/NV_\([A-Za-z_]*\)_S/NV_\1_S_LD/g' nvector_nrnserial_ld.c >temp
mv temp nvector_nrnserial_ld.c
*/

/*
 * -----------------------------------------------------------------
 * $Revision: 855 $
 * $Date: 2005-02-10 00:15:46 +0100 (Thu, 10 Feb 2005) $
 * ----------------------------------------------------------------- 
 * Programmer(s): Scott D. Cohen, Alan C. Hindmarsh, Radu Serban,
 *                and Aaron Collier @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California.
 * Produced at the Lawrence Livermore National Laboratory.
 * All rights reserved.
 * For details, see sundials/shared/LICENSE.
 * -----------------------------------------------------------------
 * This is the header file for the serial implementation of the
 * NVECTOR module.
 *
 * Part I contains declarations specific to the serial
 * implementation of the supplied NVECTOR module.
 *
 * Part II defines accessor macros that allow the user to
 * efficiently use the type N_Vector without making explicit
 * references to the underlying data structure.
 *
 * Part III contains the prototype for the constructor N_VNew_NrnSerialLD
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
 *       N_VLinearSum_NrnSerialLD(a,x,b,y,y);
 *
 *     (which stores the result of the operation a*x+b*y in y)
 *     is legal.
 * -----------------------------------------------------------------
 */

#ifndef _NVECTOR_NRNSERIAL_LD_H
#define _NVECTOR_NRNSERIAL_LD_H

#ifdef __cplusplus  /* wrapper to enable C++ usage */
extern "C" {
#endif

#include "nvector.h"
#include "sundialstypes.h"

/*
 * -----------------------------------------------------------------
 * PART I: SERIAL implementation of N_Vector
 * -----------------------------------------------------------------
 */

/* serial implementation of the N_Vector 'content' structure
   contains the length of the vector, a pointer to an array
   of realtype components, and a flag indicating ownership of
   the data */

struct _N_VectorContent_NrnSerialLD {
  long int length;
  booleantype own_data;
  realtype *data;
};

typedef struct _N_VectorContent_NrnSerialLD *N_VectorContent_NrnSerialLD;

/*
 * -----------------------------------------------------------------
 * PART II: macros NV_CONTENT_S_LD, NV_DATA_S_LD, NV_OWN_DATA_S_LD,
 *          NV_LENGTH_S_LD, and NV_Ith_S_LD
 * -----------------------------------------------------------------
 * In the descriptions below, the following user declarations
 * are assumed:
 *
 * N_Vector v;
 * long int i;
 *
 * (1) NV_CONTENT_S_LD
 *
 *     This routines gives access to the contents of the serial
 *     vector N_Vector.
 *
 *     The assignment v_cont = NV_CONTENT_S_LD(v) sets v_cont to be
 *     a pointer to the serial N_Vector content structure.
 *
 * (2) NV_DATA_S_LD NV_OWN_DATA_S_LD and NV_LENGTH_S_LD
 *
 *     These routines give access to the individual parts of
 *     the content structure of a serial N_Vector.
 *
 *     The assignment v_data = NV_DATA_S_LD(v) sets v_data to be
 *     a pointer to the first component of v. The assignment
 *     NV_DATA_S_LD(v) = data_V sets the component array of v to
 *     be data_v by storing the pointer data_v.
 *
 *     The assignment v_len = NV_LENGTH_S_LD(v) sets v_len to be
 *     the length of v. The call NV_LENGTH_S_LD(v) = len_v sets
 *     the length of v to be len_v.
 *
 * (3) NV_Ith_S_LD
 *
 *     In the following description, the components of an
 *     N_Vector are numbered 0..n-1, where n is the length of v.
 *
 *     The assignment r = NV_Ith_S_LD(v,i) sets r to be the value of
 *     the ith component of v. The assignment NV_Ith_S_LD(v,i) = r
 *     sets the value of the ith component of v to be r.
 *
 * Note: When looping over the components of an N_Vector v, it is
 * more efficient to first obtain the component array via
 * v_data = NV_DATA_S_LD(v) and then access v_data[i] within the
 * loop than it is to use NV_Ith_S_LD(v,i) within the loop.
 * -----------------------------------------------------------------
 */

#define NV_CONTENT_S_LD(v)  ( (N_VectorContent_NrnSerialLD)(v->content) )

#define NV_LENGTH_S_LD(v)   ( NV_CONTENT_S_LD(v)->length )

#define NV_OWN_DATA_S_LD(v) ( NV_CONTENT_S_LD(v)->own_data )

#define NV_DATA_S_LD(v)     ( NV_CONTENT_S_LD(v)->data )

#define NV_Ith_S_LD(v,i)    ( NV_DATA_S_LD(v)[i] )

/*
 * -----------------------------------------------------------------
 * PART III: functions exported by nvector_serial
 * 
 * CONSTRUCTORS:
 *    N_VNew_NrnSerialLD
 *    N_VNewEmpty_NrnSerialLD
 *    N_VClone_NrnSerialLD
 *    N_VCloneEmpty_NrnSerialLD
 *    N_VMake_NrnSerialLD
 *    N_VNewVectorArray_NrnSerialLD
 *    N_VNewVectorArrayEmpty_NrnSerialLD
 * DESTRUCTORS:
 *    N_VDestroy_NrnSerialLD
 *    N_VDestroyVectorArray_NrnSerialLD
 * -----------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------
 * Function : N_VNew_NrnSerialLD
 * -----------------------------------------------------------------
 * This function creates and allocates memory for a serial vector.
 * -----------------------------------------------------------------
 */

N_Vector N_VNew_NrnSerialLD(long int vec_length);

/*
 * -----------------------------------------------------------------
 * Function : N_VNewEmpty_NrnSerialLD
 * -----------------------------------------------------------------
 * This function creates a new serial N_Vector with an empty (NULL)
 * data array.
 * -----------------------------------------------------------------
 */

N_Vector N_VNewEmpty_NrnSerialLD(long int vec_length);

/*
 * -----------------------------------------------------------------
 * Function : N_VCloneEmpty_NrnSerialLD
 * -----------------------------------------------------------------
 * This function creates a new serial N_Vector with an empty (NULL)
 * data array.
 * -----------------------------------------------------------------
 */

N_Vector N_VCloneEmpty_NrnSerialLD(N_Vector w);

/*
 * -----------------------------------------------------------------
 * Function : N_VMake_NrnSerialLD
 * -----------------------------------------------------------------
 * This function creates and allocates memory for a serial vector
 * with a user-supplied data array.
 * -----------------------------------------------------------------
 */

N_Vector N_VMake_NrnSerialLD(long int vec_length, realtype *v_data);

/*
 * -----------------------------------------------------------------
 * Function : N_VNewVectorArray_NrnSerialLD
 * -----------------------------------------------------------------
 * This function creates an array of 'count' serial vectors. This
 * array of N_Vectors can be freed using N_VDestroyVectorArray
 * (defined by the generic NVECTOR module).
 * -----------------------------------------------------------------
 */

N_Vector *N_VNewVectorArray_NrnSerialLD(int count, long int vec_length);

/*
 * -----------------------------------------------------------------
 * Function : N_VNewVectorArrayEmpty_NrnSerialLD
 * -----------------------------------------------------------------
 * This function creates an array of 'count' serial vectors each
 * with an empty (NULL) data array.
 * -----------------------------------------------------------------
 */

N_Vector *N_VNewVectorArrayEmpty_NrnSerialLD(int count, long int vec_length);

/*
 * -----------------------------------------------------------------
 * Function : N_VDestroyVectorArray_NrnSerialLD
 * -----------------------------------------------------------------
 * This function frees an array of N_Vector created with 
 * N_VNewVectorArray_NrnSerialLD.
 * -----------------------------------------------------------------
 */

void N_VDestroyVectorArray_NrnSerialLD(N_Vector *vs, int count);

/*
 * -----------------------------------------------------------------
 * Function : N_VPrint_NrnSerialLD
 * -----------------------------------------------------------------
 * This function prints the content of a serial vector to stdout.
 * -----------------------------------------------------------------
 */

void N_VPrint_NrnSerialLD(N_Vector v);

/*
 * -----------------------------------------------------------------
 * serial implementations of various useful vector operations
 * -----------------------------------------------------------------
 */

N_Vector N_VClone_NrnSerialLD(N_Vector w);
void N_VDestroy_NrnSerialLD(N_Vector v);
void N_VSpace_NrnSerialLD(N_Vector v, long int *lrw, long int *liw);
realtype *N_VGetArrayPointer_NrnSerialLD(N_Vector v);
void N_VSetArrayPointer_NrnSerialLD(realtype *v_data, N_Vector v);
void N_VLinearSum_NrnSerialLD(realtype a, N_Vector x, realtype b, N_Vector y, N_Vector z);
void N_VConst_NrnSerialLD(realtype c, N_Vector z);
void N_VProd_NrnSerialLD(N_Vector x, N_Vector y, N_Vector z);
void N_VDiv_NrnSerialLD(N_Vector x, N_Vector y, N_Vector z);
void N_VScale_NrnSerialLD(realtype c, N_Vector x, N_Vector z);
void N_VAbs_NrnSerialLD(N_Vector x, N_Vector z);
void N_VInv_NrnSerialLD(N_Vector x, N_Vector z);
void N_VAddConst_NrnSerialLD(N_Vector x, realtype b, N_Vector z);
realtype N_VDotProd_NrnSerialLD(N_Vector x, N_Vector y);
realtype N_VMaxNorm_NrnSerialLD(N_Vector x);
realtype N_VWrmsNorm_NrnSerialLD(N_Vector x, N_Vector w);
realtype N_VWrmsNormMask_NrnSerialLD(N_Vector x, N_Vector w, N_Vector id);
realtype N_VMin_NrnSerialLD(N_Vector x);
realtype N_VWL2Norm_NrnSerialLD(N_Vector x, N_Vector w);
realtype N_VL1Norm_NrnSerialLD(N_Vector x);
void N_VCompare_NrnSerialLD(realtype c, N_Vector x, N_Vector z);
booleantype N_VInvTest_NrnSerialLD(N_Vector x, N_Vector z);
booleantype N_VConstrMask_NrnSerialLD(N_Vector c, N_Vector x, N_Vector m);
realtype N_VMinQuotient_NrnSerialLD(N_Vector num, N_Vector denom);

#ifdef __cplusplus
}
#endif

#endif

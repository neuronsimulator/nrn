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
 * This is the main header file for the MPI-enabled implementation
 * of the NVECTOR module.
 *
 * Part I contains declarations specific to the parallel
 * implementation of the supplied NVECTOR module.
 *
 * Part II defines accessor macros that allow the user to efficiently
 * use the type N_Vector without making explicit references to the
 * underlying data structure.
 *
 * Part III contains the prototype for the constructor
 * N_VNew_Parallel as well as implementation-specific prototypes
 * for various useful vector operations.
 *
 * Notes:
 *
 *   - The definition of the generic N_Vector structure can be
 *     found in the header file shared/include/nvector.h.
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
 *        N_VLinearSum_Parallel(a,x,b,y,y);
 *
 *     (which stores the result of the operation a*x+b*y in y)
 *     is legal.
 * -----------------------------------------------------------------
 */

#ifndef _NVECTOR_PARALLEL_H
#define _NVECTOR_PARALLEL_H

#if NRNMPI_DYNAMICLOAD
#define MPI_DOUBLE double
#define MPI_LONG long
#define MPI_Comm int
#else
#include <mpi.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#include "nvector.h"
#include "sundialstypes.h"

/*
 * -----------------------------------------------------------------
 * PART I: PARALLEL implementation of N_Vector               
 * -----------------------------------------------------------------
 */

/* define MPI data types */

#if defined(SUNDIALS_SINGLE_PRECISION)

#define PVEC_REAL_MPI_TYPE MPI_FLOAT

#elif defined(SUNDIALS_DOUBLE_PRECISION)

#define PVEC_REAL_MPI_TYPE MPI_DOUBLE

#elif defined(SUNDIALS_EXTENDED_PRECISION)

#define PVEC_REAL_MPI_TYPE MPI_LONG_DOUBLE

#endif

#define PVEC_INTEGER_MPI_TYPE MPI_LONG

/* parallel implementation of the N_Vector 'content' structure
   contains the global and local lengths of the vector, a pointer
   to an array of realtype components, the MPI communicator,
   and a flag indicating ownership of the data */

struct _N_VectorContent_Parallel {
  long int local_length;   /* local vector length         */
  long int global_length;  /* global vector length        */
  booleantype own_data;    /* ownership of data           */
  realtype *data;          /* local data array            */
  MPI_Comm comm;           /* pointer to MPI communicator */
};

typedef struct _N_VectorContent_Parallel *N_VectorContent_Parallel;

/*
 * -----------------------------------------------------------------
 * PART II: macros NV_CONTENT_P, NV_DATA_P, NV_OWN_DATA_P,
 *          NV_LOCLENGTH_P, NV_GLOBLENGTH_P,NV_COMM_P, and NV_Ith_P
 * -----------------------------------------------------------------
 * In the descriptions below, the following user declarations
 * are assumed:
 *
 * N_Vector v;
 * long int v_len, s_len, i;
 *
 * (1) NV_CONTENT_P
 *
 *     This routines gives access to the contents of the parallel
 *     vector N_Vector.
 *
 *     The assignment v_cont = NV_CONTENT_P(v) sets v_cont to be
 *     a pointer to the parallel N_Vector content structure.
 *
 * (2) NV_DATA_P, NV_OWN_DATA_P, NV_LOCLENGTH_P, NV_GLOBLENGTH_P,
 *     and NV_COMM_P
 *
 *     These routines give access to the individual parts of
 *     the content structure of a parallel N_Vector.
 *
 *     The assignment v_data = NV_DATA_P(v) sets v_data to be
 *     a pointer to the first component of the local data for
 *     the vector v. The assignment NV_DATA_P(v) = data_v sets
 *     the component array of v to be data_V by storing the
 *     pointer data_v.
 *
 *     The assignment v_llen = NV_LOCLENGTH_P(v) sets v_llen to
 *     be the length of the local part of the vector v. The call
 *     NV_LOCLENGTH_P(v) = llen_v sets the local length
 *     of v to be llen_v.
 *
 *     The assignment v_glen = NV_GLOBLENGTH_P(v) sets v_glen to
 *     be the global length of the vector v. The call
 *     NV_GLOBLENGTH_P(v) = glen_v sets the global length of v to
 *     be glen_v.
 *
 *     The assignment v_comm = NV_COMM_P(v) sets v_comm to be the
 *     MPI communicator of the vector v. The assignment
 *     NV_COMM_C(v) = comm_v sets the MPI communicator of v to be
 *     comm_v.
 *
 * (3) NV_Ith_P
 *
 *     In the following description, the components of the
 *     local part of an N_Vector are numbered 0..n-1, where n
 *     is the local length of (the local part of) v.
 *
 *     The assignment r = NV_Ith_P(v,i) sets r to be the value
 *     of the ith component of the local part of the vector v.
 *     The assignment NV_Ith_P(v,i) = r sets the value of the
 *     ith local component of v to be r.
 *
 * Note: When looping over the components of an N_Vector v, it is
 * more efficient to first obtain the component array via
 * v_data = NV_DATA_P(v) and then access v_data[i] within the
 * loop than it is to use NV_Ith_P(v,i) within the loop.
 * -----------------------------------------------------------------
 */

#define NV_CONTENT_P(v)    ( (N_VectorContent_Parallel)(v->content) )

#define NV_LOCLENGTH_P(v)  ( NV_CONTENT_P(v)->local_length )

#define NV_GLOBLENGTH_P(v) ( NV_CONTENT_P(v)->global_length )

#define NV_OWN_DATA_P(v)   ( NV_CONTENT_P(v)->own_data )

#define NV_DATA_P(v)       ( NV_CONTENT_P(v)->data )

#define NV_COMM_P(v)       ( NV_CONTENT_P(v)->comm )

#define NV_Ith_P(v,i)      ( NV_DATA_P(v)[i] )

/*
 * -----------------------------------------------------------------
 * PART III: functions exported by nvector_parallel
 * 
 * CONSTRUCTORS:
 *    N_VNew_Parallel
 *    N_VNewEmpty_Parallel
 *    N_VClone_Parallel
 *    N_VCloneEmpty_Parallel
 *    N_VMake_Parallel
 *    N_VNewVectorArray_Parallel
 *    N_VNewVectorArrayEmpty_Parallel
 * DESTRUCTORS:
 *    N_VDestroy_Parallel
 *    N_VDestroyVectorArray_Parallel
 * -----------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------
 * Function : N_VNew_Parallel
 * -----------------------------------------------------------------
 * This function creates and allocates memory for a parallel vector.
 * -----------------------------------------------------------------
 */

N_Vector N_VNew_Parallel(MPI_Comm comm, 
                         long int local_length,
                         long int global_length);

/*
 * -----------------------------------------------------------------
 * Function : N_VNewEmpty_Parallel
 * -----------------------------------------------------------------
 * This function creates a new parallel N_Vector with an empty
 * (NULL) data array.
 * -----------------------------------------------------------------
 */

N_Vector N_VNewEmpty_Parallel(MPI_Comm comm, 
                              long int local_length,
                              long int global_length);

/*
 * -----------------------------------------------------------------
 * Function : N_VCloneEmpty_Parallel
 * -----------------------------------------------------------------
 * This function creates a new parallel N_Vector with an empty (NULL)
 * data array using the vector w as a template
 * (sets own_data = FALSE).
 * -----------------------------------------------------------------
 */

N_Vector N_VCloneEmpty_Parallel(N_Vector w);

/*
 * -----------------------------------------------------------------
 * Function : N_VMake_Parallel
 * -----------------------------------------------------------------
 * This function creates and allocates memory for a parallel vector
 * with a user-supplied data array.
 * -----------------------------------------------------------------
 */

N_Vector N_VMake_Parallel(MPI_Comm comm, 
                          long int local_length,
                          long int global_length,
                          realtype *v_data);

/*
 * -----------------------------------------------------------------
 * Function : N_VNewVectorArray_Parallel
 * -----------------------------------------------------------------
 * This function creates an array of 'count' parallel vectors. This
 * array of N_Vectors can be freed using N_VDestroyVectorArray
 * (defined by the generic NVECTOR module).
 * -----------------------------------------------------------------
 */

N_Vector *N_VNewVectorArray_Parallel(int count, 
                                     MPI_Comm comm, 
                                     long int local_length,
                                     long int global_length);

/*
 * -----------------------------------------------------------------
 * Function : N_VNewVectorArrayEmpty_Parallel
 * -----------------------------------------------------------------
 * This function creates an array of 'count' parallel vectors each 
 * with an empty (NULL) data array.
 * -----------------------------------------------------------------
 */

N_Vector *N_VNewVectorArrayEmpty_Parallel(int count, 
                                          MPI_Comm comm, 
                                          long int local_length,
                                          long int global_length);

/*
 * -----------------------------------------------------------------
 * Function : N_VDestroyVectorArray_Parallel
 * -----------------------------------------------------------------
 * This function frees an array of N_Vector created with 
 * N_VNewVectorArray_Parallel.
 * -----------------------------------------------------------------
 */

void N_VDestroyVectorArray_Parallel(N_Vector *vs, int count);

/*
 * -----------------------------------------------------------------
 * Function : N_VPrint_Parallel
 * -----------------------------------------------------------------
 * This function prints the content of a parallel vector to stdout.
 * -----------------------------------------------------------------
 */

void N_VPrint_Parallel(N_Vector v);

/*
 * -----------------------------------------------------------------
 * parallel implementations of the vector operations
 * -----------------------------------------------------------------
 */

N_Vector N_VClone_Parallel(N_Vector w);
void N_VDestroy_Parallel(N_Vector v);
void N_VSpace_Parallel(N_Vector v, long int *lrw, long int *liw);
realtype *N_VGetArrayPointer_Parallel(N_Vector v);
void N_VSetArrayPointer_Parallel(realtype *v_data, N_Vector v);
void N_VLinearSum_Parallel(realtype a, N_Vector x, realtype b, N_Vector y, N_Vector z);
void N_VConst_Parallel(realtype c, N_Vector z);
void N_VProd_Parallel(N_Vector x, N_Vector y, N_Vector z);
void N_VDiv_Parallel(N_Vector x, N_Vector y, N_Vector z);
void N_VScale_Parallel(realtype c, N_Vector x, N_Vector z);
void N_VAbs_Parallel(N_Vector x, N_Vector z);
void N_VInv_Parallel(N_Vector x, N_Vector z);
void N_VAddConst_Parallel(N_Vector x, realtype b, N_Vector z);
realtype N_VDotProd_Parallel(N_Vector x, N_Vector y);
realtype N_VMaxNorm_Parallel(N_Vector x);
realtype N_VWrmsNorm_Parallel(N_Vector x, N_Vector w);
realtype N_VWrmsNormMask_Parallel(N_Vector x, N_Vector w, N_Vector id);
realtype N_VMin_Parallel(N_Vector x);
realtype N_VWL2Norm_Parallel(N_Vector x, N_Vector w);
realtype N_VL1Norm_Parallel(N_Vector x);
void N_VCompare_Parallel(realtype c, N_Vector x, N_Vector z);
booleantype N_VInvTest_Parallel(N_Vector x, N_Vector z);
booleantype N_VConstrMask_Parallel(N_Vector c, N_Vector x, N_Vector m);
realtype N_VMinQuotient_Parallel(N_Vector num, N_Vector denom);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif

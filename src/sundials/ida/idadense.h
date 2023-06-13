/*
 * -----------------------------------------------------------------
 * $Revision: 855 $
 * $Date: 2005-02-10 00:15:46 +0100 (Thu, 10 Feb 2005) $
 * ----------------------------------------------------------------- 
 * Programmer(s): Alan C. Hindmarsh and Radu Serban @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California.
 * Produced at the Lawrence Livermore National Laboratory.
 * All rights reserved.
 * For details, see sundials/ida/LICENSE.
 * -----------------------------------------------------------------
 * This is the header file for the IDA/IDAS dense linear solver
 * module, IDADENSE.
 * -----------------------------------------------------------------
 */

#ifndef _IDADENSE_H
#define _IDADENSE_H

#ifdef __cplusplus  /* wrapper to enable C++ usage */
extern "C" {
#endif

#include <stdio.h>

#include <sundials/shared/dense.h>
#include <sundials/shared/nvector.h>
#include <sundials/shared/sundialstypes.h>

/*
 * -----------------------------------------------------------------
 * Type : IDADenseJacFn
 * -----------------------------------------------------------------
 * A dense Jacobian approximation function djac must have the     
 * prototype given below. Its parameters are:                     
 *                                                                
 * Neq is the problem size, and length of all vector arguments.   
 *                                                                
 * tt  is the current value of the independent variable t.        
 *                                                                
 * yy  is the current value of the dependent variable vector,     
 *     namely the predicted value of y(t).                     
 *                                                                
 * yp  is the current value of the derivative vector y',          
 *     namely the predicted value of y'(t).                    
 *                                                                
 * rr  is the residual vector F(tt,yy,yp).                     
 *                                                                
 * c_j is the scalar in the system Jacobian, proportional to 1/hh.
 *                                                                
 * jac_data  is a pointer to user Jacobian data - the same as the    
 *     jdata parameter passed to IDADense.                     
 *                                                                
 * Jac is the dense matrix (of type DenseMat) to be loaded by  
 *     an IDADenseJacFn routine with an approximation to the   
 *     system Jacobian matrix                                  
 *            J = dF/dy + c_j*dF/dy'                            
 *     at the given point (t,y,y'), where the DAE system is    
 *     given by F(t,y,y') = 0.  Jac is preset to zero, so only 
 *     the nonzero elements need to be loaded.  See note below.
 *                                                                
 * tmp1, tmp2, tmp3 are pointers to memory allocated for          
 *     N_Vectors which can be used by an IDADenseJacFn routine 
 *     as temporary storage or work space.                     
 *                                                                
 * NOTE: The following are two efficient ways to load Jac:         
 * (1) (with macros - no explicit data structure references)      
 *     for (j=0; j < Neq; j++) {                                  
 *       col_j = DENSE_COL(Jac,j);                                 
 *       for (i=0; i < Neq; i++) {                                
 *         generate J_ij = the (i,j)th Jacobian element           
 *         col_j[i] = J_ij;                                       
 *       }                                                        
 *     }                                                          
 * (2) (without macros - explicit data structure references)      
 *     for (j=0; j < Neq; j++) {                                  
 *       col_j = (Jac->data)[j];                                   
 *       for (i=0; i < Neq; i++) {                                
 *         generate J_ij = the (i,j)th Jacobian element           
 *         col_j[i] = J_ij;                                       
 *       }                                                        
 *     }                                                          
 * A third way, using the DENSE_ELEM(A,i,j) macro, is much less   
 * efficient in general.  It is only appropriate for use in small 
 * problems in which efficiency of access is NOT a major concern. 
 *                                                                
 * NOTE: If the user's Jacobian routine needs other quantities,   
 *     they are accessible as follows: hcur (the current stepsize)
 *     and ewt (the error weight vector) are accessible through   
 *     IDAGetCurrentStep and IDAGetErrWeights, respectively (see  
 *     ida.h). The unit roundoff is available as                  
 *     UNIT_ROUNDOFF defined in sundialstypes.h                   
 *                                                                
 * The IDADenseJacFn should return                                
 *     0 if successful,                                           
 *     a positive int if a recoverable error occurred, or         
 *     a negative int if a nonrecoverable error occurred.         
 * In the case of a recoverable error return, the integrator will 
 * attempt to recover by reducing the stepsize (which changes cj).
 * -----------------------------------------------------------------
 */
  
typedef int (*IDADenseJacFn)(long int Neq, realtype tt, 
                             N_Vector yy, N_Vector yp, N_Vector rr,
                             realtype c_j, void *jac_data, 
                             DenseMat Jac, 
                             N_Vector tmp1, N_Vector tmp2, 
                             N_Vector tmp3);

/*
 * -----------------------------------------------------------------
 * Function : IDADense
 * -----------------------------------------------------------------
 * A call to the IDADense function links the main integrator      
 * with the IDADENSE linear solver module.                        
 *                                                                
 * ida_mem is the pointer to integrator memory returned by        
 *     IDACreate.                                                 
 *                                                                
 * Neq  is the problem size                                       
 *                                                                
 * IDADense returns:                                              
 *     IDADENSE_SUCCESS   = 0  if successful                              
 *     IDADENSE_LMEM_FAIL = -1 if there was a memory allocation failure   
 *     IDADENSE_ILL_INPUT = -2 if NVECTOR found incompatible           
 *                                                                
 * NOTE: The dense linear solver assumes a serial implementation  
 *       of the NVECTOR package. Therefore, IDADense will first
 *       test for a compatible N_Vector internal representation
 *       by checking that the functions N_VGetArrayPointer and
 *       N_VSetArrayPointer exist.
 * -----------------------------------------------------------------
 */

int IDADense(void *ida_mem, long int Neq); 

/*
 * -----------------------------------------------------------------
 * Optional inputs to the IDADENSE linear solver
 * -----------------------------------------------------------------
 * IDADenseSetJacFn specifies the dense Jacobian approximation    
 *        routine to be used. A user-supplied djac routine must   
 *        be of type IDADenseJacFn.                               
 *        By default, a difference quotient routine IDADenseDQJac,
 *        supplied with this solver is used.                      
 * IDADenseSetJacData specifies a pointer to user data which is   
 *        passed to the djac routine every time it is called.     
 *                                                                
 * The return value of IDADenseSet* is one of:
 *    IDADENSE_SUCCESS   if successful
 *    IDADENSE_MEM_NULL  if the ida memory was NULL
 *    IDaDENSE_LMEM_NULL if the idadense memory was NULL
 * -----------------------------------------------------------------
 */

int IDADenseSetJacFn(void *ida_mem, IDADenseJacFn djac);
int IDADenseSetJacData(void *ida_mem, void *jac_data);
 
/*
 * -----------------------------------------------------------------
 * Optional outputs from the IDADENSE linear solver
 * -----------------------------------------------------------------
 * IDADenseGetWorkSpace returns the real and integer workspace used 
 *     by IDADENSE.                                                  
 * IDADenseGetNumJacEvals returns the number of calls made to the 
 *     Jacobian evaluation routine djac.                          
 * IDADenseGetNumResEvals returns the number of calls to the user 
 *     res routine due to finite difference Jacobian evaluation.  
 * IDADenseGetLastFlag returns the last error flag set by any of
 *     the IDADENSE interface functions.
 *
 * The return value of IDADenseGet* is one of:
 *    IDADENSE_SUCCESS   if successful
 *    IDADENSE_MEM_NULL  if the ida memory was NULL
 *    IDaDENSE_LMEM_NULL if the idadense memory was NULL
 * -----------------------------------------------------------------
 */

int IDADenseGetWorkSpace(void *ida_mem, long int *lenrwD, long int *leniwD);
int IDADenseGetNumJacEvals(void *ida_mem, long int *njevalsD);
int IDADenseGetNumResEvals(void *ida_mem, long int *nrevalsD);
int IDADenseGetLastFlag(void *ida_mem, int *flag);

/* IDADENSE return values */

#define IDADENSE_SUCCESS    0
#define IDADENSE_MEM_NULL  -1 
#define IDADENSE_LMEM_NULL -2 
#define IDADENSE_ILL_INPUT -3
#define IDADENSE_MEM_FAIL  -4

#ifdef __cplusplus
}
#endif

#endif

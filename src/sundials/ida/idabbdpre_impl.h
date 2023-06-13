/*
 * -----------------------------------------------------------------
 * $Revision: 855 $
 * $Date: 2005-02-10 00:15:46 +0100 (Thu, 10 Feb 2005) $
 * ----------------------------------------------------------------- 
 * Programmers: Alan C. Hindmarsh and Radu Serban @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California  
 * Produced at the Lawrence Livermore National Laboratory
 * All rights reserved
 * For details, see sundials/ida/LICENSE
 * -----------------------------------------------------------------
 * This is the header file (private version) for the IDABBDPRE
 * module, for a band-block-diagonal preconditioner, i.e. a
 * block-diagonal matrix with banded blocks, for use with IDA/IDAS
 * and IDASpgmr.  
 * -----------------------------------------------------------------
 */

#ifndef _IBBDPRE_IMPL_H
#define _IBBDPRE_IMPL_H

#ifdef __cplusplus  /* wrapper to enable C++ usage */
extern "C" {
#endif

#include "idabbdpre.h"

#include "band.h"
#include "iterative.h"
#include <sundials/shared/nvector.h>
#include <sundials/shared/sundialstypes.h>

/*
 * -----------------------------------------------------------------
 * Prototypes of IDABBDPrecSetup and IDABBDPrecSolve 
 * -----------------------------------------------------------------
 */

int IDABBDPrecSetup(realtype tt, 
		    N_Vector yy, N_Vector yp, N_Vector rr, 
		    realtype c_j, void *prec_data,
		    N_Vector tmp1, N_Vector tmp2, N_Vector tmp3);
 
int IDABBDPrecSolve(realtype tt, 
		    N_Vector yy, N_Vector yp, N_Vector rr, 
		    N_Vector rvec, N_Vector zvec,
		    realtype c_j, realtype delta, void *prec_data, 
                    N_Vector tmp);

/*
 * -----------------------------------------------------------------
 * Definition of IBBDPrecData 
 * -----------------------------------------------------------------
 */

typedef struct {

  /* passed by user to IDABBDPrecAlloc, used by 
     IDABBDPrecSetup/IDABBDPrecSolve functions: */
  long int mudq, mldq, mukeep, mlkeep;
  realtype rel_yy;
  IDABBDLocalFn glocal;
  IDABBDCommFn gcomm;

 /* allocated for use by IDABBDPrecSetup */
  N_Vector tempv4;

  /* set by IDABBDPrecon and used by IDABBDPrecSolve: */
  BandMat PP;
  long int *pivots;

  /* set by IDABBDPrecAlloc and used by IDABBDPrecSetup */
  long int n_local;

  /* available for optional output: */
  long int rpwsize;
  long int ipwsize;
  long int nge;

  /* Pointer to ida_mem */
  IDAMem IDA_mem;

} *IBBDPrecData;

/*
 * -----------------------------------------------------------------
 * Error Messages
 * -----------------------------------------------------------------
 */

#define MSGBBD_IDAMEM_NULL  "IBBDPrecAlloc-- integrator memory is NULL.\n\n"
#define MSGBBD_BAD_NVECTOR  "IBBDPrecAlloc-- a required vector operation is not implemented.\n\n"
#define MSGBBD_WRONG_NVEC   "IBBDPrecAlloc-- incompatible NVECTOR implementation.\n\n"
#define MSGBBD_PDATA_NULL   "IBBDPrecGet*-- BBDPrecData is NULL. \n\n"

#define MSGBBD_NO_PDATA     "IBBDSpgmr-- BBDPrecData is NULL. \n\n"

#ifdef __cplusplus
}
#endif

#endif

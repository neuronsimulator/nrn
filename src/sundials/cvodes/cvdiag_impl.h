/*
 * -----------------------------------------------------------------
 * $Revision: 855 $
 * $Date: 2005-02-10 00:15:46 +0100 (Thu, 10 Feb 2005) $
 * ----------------------------------------------------------------- 
 * Programmer(s): Scott D. Cohen, Alan C. Hindmarsh and
 *                Radu Serban @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California.
 * Produced at the Lawrence Livermore National Laboratory.
 * All rights reserved.
 * For details, see sundials/cvodes/LICENSE.
 * -----------------------------------------------------------------
 * Implementation header file for the diagonal linear solver, CVDIAG.
 * -----------------------------------------------------------------
 */

#ifndef _CVDIAG_IMPL_H
#define _CVDIAG_IMPL_H

#ifdef __cplusplus  /* wrapper to enable C++ usage */
extern "C" {
#endif

#include <stdio.h>

#include "cvdiag.h"

#include <sundials/shared/nvector.h>
#include <sundials/shared/sundialstypes.h>

/*
 * -----------------------------------------------------------------
 * Types: CVDiagMemRec, CVDiagMem
 * -----------------------------------------------------------------
 * The type CVDiagMem is pointer to a CVDiagMemRec.
 * This structure contains CVDiag solver-specific data.
 * -----------------------------------------------------------------
 */

typedef struct {

  realtype di_gammasv; /* gammasv = gamma at the last call to setup */
                       /* or solve                                  */

  N_Vector di_M;       /* M = (I - gamma J)^{-1} , gamma = h / l1   */

  N_Vector di_bit;     /* temporary storage vector                  */

  N_Vector di_bitcomp; /* temporary storage vector                  */

  long int di_nfeDI;   /* no. of calls to f due to difference 
                          quotient diagonal Jacobian approximation  */

  int di_last_flag;    /* last error return flag                    */

} CVDiagMemRec, *CVDiagMem;

/* Error Messages */

#define _CVDIAG_          "CVDiag-- "
#define MSGDG_CVMEM_NULL  _CVDIAG_ "Integrator memory is NULL.\n\n"
#define MSGDG_BAD_NVECTOR _CVDIAG_ "A required vector operation is not implemented.\n\n"
#define MSGDG_MEM_FAIL    _CVDIAG_ "A memory request failed.\n\n"

#define MSGDG_SETGET_CVMEM_NULL "CVDiagGet*-- Integrator memory is NULL.\n\n"

#define MSGDG_SETGET_LMEM_NULL "CVDiagGet*-- cvdiag memory is NULL.\n\n"

#ifdef __cplusplus
}
#endif

#endif

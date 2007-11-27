/*
 * -----------------------------------------------------------------
 * $Revision: 855 $
 * $Date: 2005-02-10 00:15:46 +0100 (Thu, 10 Feb 2005) $
 * ----------------------------------------------------------------- 
 * Programmer(s): Radu Serban @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California.
 * Produced at the Lawrence Livermore National Laboratory.
 * All rights reserved.
 * For details, see sundials/shared/LICENSE.
 * -----------------------------------------------------------------
 * This file (companion of nvector_serial.h) contains the
 * implementation needed for the Fortran initialization of serial
 * vector operations.
 * -----------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>

#include "fnvector_serial.h"
#include "nvector_serial.h"
#include "sundialstypes.h"

/* Define global variable F2C_vec */
N_Vector F2C_vec;

/* Fortran callable interfaces */

void FNV_INITS(long int *neq, int *ier)
{
 F2C_vec = N_VNew_Serial(*neq);

 *ier = (F2C_vec == NULL) ? -1 : 0 ;
}

void FNV_FREES(void)
{
  N_VDestroy_Serial(F2C_vec);
}


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
 * implementation needed for the Fortran initialization of parallel 
 * vector operations.
 * -----------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>

#include "fnvector_parallel.h"
#include "mpi.h"
#include "nvector_parallel.h"
#include <sundials/shared/sundialstypes.h>

/* Define global variable F2C_vec */
N_Vector F2C_vec;

/* Fortran callable interfaces */

void FNV_INITP(long int *nlocal, long int *nglobal, int *ier)
{
  /* Call N_VNew_Parallel:
     the first slot is for the communicator. 
     (From Fortran, only MPI_COMM_WORLD is allowed)
     *nlocal  is the local vector length
     *nglobal is the global vector length */

  F2C_vec = N_VNew_Parallel(MPI_COMM_WORLD, *nlocal, *nglobal);

 *ier = (F2C_vec == NULL) ? -1 : 0 ;
}

void FNV_FREEP(void)
{
  N_VDestroy_Parallel(F2C_vec);
}

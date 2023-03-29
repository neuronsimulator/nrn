/*
 * -----------------------------------------------------------------
 * $Revision: 855 $
 * $Date: 2005-02-10 00:15:46 +0100 (Thu, 10 Feb 2005) $
 * -----------------------------------------------------------------
 * Programmer(s): Scott D. Cohen, Alan C. Hindmarsh and
 *                Aaron Collier @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California.
 * Produced at the Lawrence Livermore National Laboratory.
 * All rights reserved.
 * For details, see sundials/shared/LICENSE.
 * -----------------------------------------------------------------
 * This is the implementation file for a simple C-language math
 * library.
 * -----------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "sundialsmath.h"
#include "sundialstypes.h"

#define ZERO RCONST(0.0)
#define ONE  RCONST(1.0)
#define TWO  RCONST(2.0)

realtype RPowerI(realtype base, int exponent)
{
  int i, expt;
  realtype prod;

  prod = ONE;
  expt = abs(exponent);
  for(i = 1; i <= expt; i++) prod *= base;
  if (exponent < 0) prod = ONE/prod;
  return(prod);
}

realtype RPowerR(realtype base, realtype exponent)
{
  if (base <= ZERO) return(ZERO);

  return(powf(base, exponent));
}

realtype RSqrt(realtype x)
{
  if (x <= ZERO) return(ZERO);

  return(sqrtf(x));
}

realtype RAbs(realtype x)
{
  return(fabsf(x));
}

realtype RPower2(realtype x)
{
  return(powf(x, TWO));
}

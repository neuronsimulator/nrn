/*
The copyrighted code from  Numerical Recipes Software has been removed
and replaced by an independent implementation found in the random.cpp file
in function Ranint4
from http://www.inference.phy.cam.ac.uk/bayesys/BayesSys.tar.gz
by John Skilling
http://www.inference.phy.cam.ac.uk/bayesys/
The code fragment was further modified by Michael Hines to change prototypes
and produce output identical to the old version. This code is now
placed under the General GNU Public License, version 2. The random.cpp file
contained the header:
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Filename:  random.cpp
//
// Purpose:   Random utility procedures for BayeSys3.
//
// History:   Random.cpp 17 Nov 1994 - 13 Sep 2003
//
// Acknowledgments:
//   "Numerical Recipes", Press et al, for ideas
//   "Handbook of Mathematical Functions", Abramowitz and Stegun, for formulas
//    Peter J Acklam website, for inverse normal code
//-----------------------------------------------------------------------------
    Copyright (c) 1994-2003 Maximum Entropy Data Consultants Ltd,
                            114c Milton Road, Cambridge CB4 1XE, England

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include "mcran4.h"

static uint32_t lowindex = 0;

double mcell_lowindex() {
    return static_cast<double>(lowindex);
}

void mcell_ran4_init(uint32_t low) {
    lowindex = low;
}

double mcell_ran4(uint32_t* high, double* x, unsigned int n, double range) {
    int i;
    for (i = 0; i < n; i++) {
        x[i] = range * nrnRan4dbl(high, lowindex);
    }
    return x[0];
}

double mcell_ran4a(uint32_t* high) {
    return nrnRan4dbl(high, lowindex);
}

uint32_t mcell_iran4(uint32_t* high) {
    return nrnRan4int(high, lowindex);
}

uint32_t nrnRan4int(uint32_t* idx1, uint32_t idx2) {
    uint32_t u, v, w, m, n;
    /* 64-bit hash */
    n = (*idx1)++;
    m = idx2;

    w = n ^ 0xbaa96887;
    v = w >> 16;
    w &= 0xffff;
    u = ~((v - w) * (v + w));
    /*m ^= (((u >> 16) | (u << 16)) ^ 0xb4f0c4a7) + w * v;*/
    m ^= (((u >> 16) | (u << 16)) ^ 0x4b0f3b58) + w * v;

    w = m ^ 0x1e17d32c;
    v = w >> 16;
    w &= 0xffff;
    u = ~((v - w) * (v + w));
    /*n ^= (((u >> 16) | (u << 16)) ^ 0x178b0f3c) + w * v;*/
    n ^= (((u >> 16) | (u << 16)) ^ 0xe874f0c3) + w * v;
    return n;

    w = n ^ 0x03bcdc3c;
    v = w >> 16;
    w &= 0xffff;
    u = (v - w) * (v + w);
    m ^= (((u >> 16) | (u << 16)) ^ 0x96aa3a59) + w * v;

    w = m ^ 0x0f33d1b2;
    v = w >> 16;
    w &= 0xffff;
    u = (v - w) * (v + w);
    n ^= (((u >> 16) | (u << 16)) ^ 0xaa5835b9) + w * v;
    return n;
}

/*
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Function:  Randouble (hines now nrnRan4dbl
//
// Purpose:   Random double-precision floating-point sample.
//            The 2^52 allowed values are odd multiples of 2^-53,
//            symmetrically placed in strict interior of (0,1).
//
// Notes: (1) Tuned to 52-bit mantissa in "double" format.
//        (2) Uses one call to Ranint to get 64 random bits, with extra
//            random integer available in Rand[3].
//        (3) All floating-point random calls are directed through this code,
//            except Rangauss which uses the extra random integer in Rand[3].
//
// History:   John Skilling   6 May 1995, 3 Dec 1995, 24 Aug 1996
//                           20 Oct 2002, 17 Dec 2002
//-----------------------------------------------------------------------------
//
*/
static const double SHIFT32 = 1.0 / 4294967296.0; /* 2^-32 */
double nrnRan4dbl(uint32_t* idx1, uint32_t idx2) {
    uint32_t hi, lo, extra;
    hi = (uint32_t) nrnRan4int(idx1, idx2); /*top 32 bits*/
                                            /*
                                            //    lo = (extra                               // low bits
                                            //                  & 0xfffff000) ^ 0x00000800;   // switch lowest (2^-53) bit ON
                                            //    return  ((double)hi + (double)lo * SHIFT32) * SHIFT32;
                                            */
    return ((double) hi) * SHIFT32;
}

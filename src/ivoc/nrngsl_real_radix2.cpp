/* fft/real_radix2.cpp
 * 
 * Copyright (C) 1996, 1997, 1998, 1999, 2000, 2007 Brian Gough
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/* Hines: for greater self-containment */
#include "nrngsl.h"
/* from gsl/fft/factorize.cpp */
static int
fft_binary_logn (const size_t n)
{
  size_t ntest ;
  size_t binary_logn = 0 ;
  size_t k = 1;

  while (k < n)
    {
      k *= 2;
      binary_logn++;
    }

  ntest = (1 << binary_logn) ;

  if (n != ntest )
    {
      return -1 ; /* n is not a power of 2 */
    }

  return binary_logn;
}

/* from gsl/fft/bitreverse.cpp */
static int
FUNCTION(fft_real,bitreverse_order) (BASE data[],
                                const size_t stride,
                                const size_t n,
                                size_t logn)
{
  /* This is the Goldrader bit-reversal algorithm */

  size_t i;
  size_t j = 0;

  logn = 0 ; /* not needed for this algorithm */

  for (i = 0; i < n - 1; i++)
    {
      size_t k = n / 2 ;

      if (i < j)
        {
          const BASE tmp = VECTOR(data,stride,i);
          VECTOR(data,stride,i) = VECTOR(data,stride,j);
          VECTOR(data,stride,j) = tmp;
        }

      while (k <= j)
        {
          j = j - k ;
          k = k / 2 ;
        }

      j += k ;
    }

  return 0;
}
/*-----------------------------------------*/

int
FUNCTION(gsl_fft_real,radix2_transform) (BASE data[], const size_t stride,  const size_t n)
{
  int result ;
  size_t p, p_1, q;
  size_t i; 
  size_t logn = 0;
  int status;

  if (n == 1) /* identity operation */
    {
      return 0 ;
    }

  /* make sure that n is a power of 2 */

  result = fft_binary_logn(n) ;

  if (result == -1) 
    {
      GSL_ERROR ("n is not a power of 2", GSL_EINVAL);
    } 
  else 
    {
      logn = result ;
    }

  /* bit reverse the ordering of input data for decimation in time algorithm */
  
  status = FUNCTION(fft_real,bitreverse_order)(data, stride, n, logn) ;

  /* apply fft recursion */

  p = 1; q = n ;

  for (i = 1; i <= logn; i++)
    {
      size_t a, b;

      p_1 = p ;
      p = 2 * p ;
      q = q / 2 ;

      /* a = 0 */

      for (b = 0; b < q; b++)
        {
          ATOMIC t0_real = VECTOR(data,stride,b*p) + VECTOR(data,stride,b*p + p_1) ;
          ATOMIC t1_real = VECTOR(data,stride,b*p) - VECTOR(data,stride,b*p + p_1) ;
          
          VECTOR(data,stride,b*p) = t0_real ;
          VECTOR(data,stride,b*p + p_1) = t1_real ;
        }

      /* a = 1 ... p_{i-1}/2 - 1 */

      {
        ATOMIC w_real = 1.0;
        ATOMIC w_imag = 0.0;

        const double theta = - 2.0 * M_PI / p;
        
        const ATOMIC s = sin (theta);
        const ATOMIC t = sin (theta / 2.0);
        const ATOMIC s2 = 2.0 * t * t;
        
        for (a = 1; a < (p_1)/2; a++)
          {
            /* trignometric recurrence for w-> exp(i theta) w */
            
            {
              const ATOMIC tmp_real = w_real - s * w_imag - s2 * w_real;
              const ATOMIC tmp_imag = w_imag + s * w_real - s2 * w_imag;
              w_real = tmp_real;
              w_imag = tmp_imag;
            }
            
            for (b = 0; b < q; b++)
              {
                ATOMIC z0_real = VECTOR(data,stride,b*p + a) ;
                ATOMIC z0_imag = VECTOR(data,stride,b*p + p_1 - a) ;
                ATOMIC z1_real = VECTOR(data,stride,b*p + p_1 + a) ;
                ATOMIC z1_imag = VECTOR(data,stride,b*p + p - a) ;
                
                /* t0 = z0 + w * z1 */
                
                ATOMIC t0_real = z0_real + w_real * z1_real - w_imag * z1_imag;
                ATOMIC t0_imag = z0_imag + w_real * z1_imag + w_imag * z1_real;
                
                /* t1 = z0 - w * z1 */
                
                ATOMIC t1_real = z0_real - w_real * z1_real + w_imag * z1_imag;
                ATOMIC t1_imag = z0_imag - w_real * z1_imag - w_imag * z1_real;
                
                VECTOR(data,stride,b*p + a) = t0_real ;
                VECTOR(data,stride,b*p + p - a) = t0_imag ;
                
                VECTOR(data,stride,b*p + p_1 - a) = t1_real ;
                VECTOR(data,stride,b*p + p_1 + a) = -t1_imag ;
              }
          }
      }

      if (p_1 >  1) 
        {
          for (b = 0; b < q; b++) 
            {
              /* a = p_{i-1}/2 */
              
              VECTOR(data,stride,b*p + p - p_1/2) *= -1 ;
            }
        }
    }
  return 0;
}

#include <nrnconf.h>

/**************************************************************************
**
** Copyright (C) 1993 David E. Steward & Zbigniew Leyk, all rights reserved.
**
**			     Meschach Library
** 
** This Meschach Library is provided "as is" without any express 
** or implied warranty of any kind with respect to this software. 
** In particular the authors shall not be liable for any direct, 
** indirect, special, incidental or consequential damages arising 
** in any way from use of the software.
** 
** Everyone is granted permission to copy, modify and redistribute this
** Meschach Library, provided:
**  1.  All copies contain this copyright notice.
**  2.  All modified copies shall carry a notice stating who
**      made the last modification and the date of such modification.
**  3.  No charge is made for this software or works derived from it.  
**      This clause shall not be construed as constraining other software
**      distributed on the same medium as this software, nor is a
**      distribution fee considered a charge.
**
***************************************************************************/


/*
  This file contains basic routines which are used by the functions
  involving complex vectors.
  These are the routines that should be modified in order to take
  full advantage of specialised architectures (pipelining, vector
  processors etc).
  */
static	char	*rcsid = "zmachine.c,v 1.1 1997/12/04 17:56:10 hines Exp";

#include	<mesch/machine.h>
#include        <mesch/zmatrix.h>
#include	<math.h>


/* __zconj__ -- complex conjugate */
void	__zconj__(zp,len)
complex	*zp;
int	len;
{
    int		i;

    for ( i = 0; i < len; i++ )
	zp[i].im = - zp[i].im;
}

/* __zip__ -- inner product
	-- computes sum_i zp1[i].zp2[i] if flag == 0
		    sum_i zp1[i]*.zp2[i] if flag != 0 */
complex	__zip__(zp1,zp2,len,flag)
complex	*zp1, *zp2;
int	flag, len;
{
    complex	sum;
    int		i;

    sum.re = sum.im = 0.0;
    if ( flag )
    {
	for ( i = 0; i < len; i++ )
	{
	    sum.re += zp1[i].re*zp2[i].re + zp1[i].im*zp2[i].im;
	    sum.im += zp1[i].re*zp2[i].im - zp1[i].im*zp2[i].re;
	}
    }
    else
    {
	for ( i = 0; i < len; i++ )
	{
	    sum.re += zp1[i].re*zp2[i].re - zp1[i].im*zp2[i].im;
	    sum.im += zp1[i].re*zp2[i].im + zp1[i].im*zp2[i].re;
	}
    }

    return sum;
}

/* __zmltadd__ -- scalar multiply and add i.e. complex saxpy
	-- computes zp1[i] += s.zp2[i]  if flag == 0
	-- computes zp1[i] += s.zp2[i]* if flag != 0 */
void	__zmltadd__(zp1,zp2,s,len,flag)
complex	*zp1, *zp2, s;
int	flag, len;
{
    int		i;
    LongReal	t_re, t_im;

    if ( ! flag )
    {
	for ( i = 0; i < len; i++ )
	{
	    t_re = zp1[i].re + s.re*zp2[i].re - s.im*zp2[i].im;
	    t_im = zp1[i].im + s.re*zp2[i].im + s.im*zp2[i].re;
	    zp1[i].re = t_re;
	    zp1[i].im = t_im;
	}
    }
    else
    {
	for ( i = 0; i < len; i++ )
	{
	    t_re = zp1[i].re + s.re*zp2[i].re + s.im*zp2[i].im;
	    t_im = zp1[i].im - s.re*zp2[i].im + s.im*zp2[i].re;
	    zp1[i].re = t_re;
	    zp1[i].im = t_im;
	}
    }
}

/* __zmlt__ scalar complex multiply array c.f. sv_mlt() */
void	__zmlt__(zp,s,out,len)
complex	*zp, s, *out;
register int	len;
{
    int		i;
    LongReal	t_re, t_im;

    for ( i = 0; i < len; i++ )
    {
	t_re = s.re*zp[i].re - s.im*zp[i].im;
	t_im = s.re*zp[i].im + s.im*zp[i].re;
	out[i].re = t_re;
	out[i].im = t_im;
    }
}

/* __zadd__ -- add complex arrays c.f. v_add() */
void	__zadd__(zp1,zp2,out,len)
complex	*zp1, *zp2, *out;
int	len;
{
    int		i;
    for ( i = 0; i < len; i++ )
    {
	out[i].re = zp1[i].re + zp2[i].re;
	out[i].im = zp1[i].im + zp2[i].im;
    }
}

/* __zsub__ -- subtract complex arrays c.f. v_sub() */
void	__zsub__(zp1,zp2,out,len)
complex	*zp1, *zp2, *out;
int	len;
{
    int		i;
    for ( i = 0; i < len; i++ )
    {
	out[i].re = zp1[i].re - zp2[i].re;
	out[i].im = zp1[i].im - zp2[i].im;
    }
}

/* __zzero__ -- zeros an array of complex numbers */
void	__zzero__(zp,len)
complex	*zp;
int	len;
{
    /* if a Real precision zero is equivalent to a string of nulls */
    MEM_ZERO((char *)zp,len*sizeof(complex));
    /* else, need to zero the array entry by entry */
    /******************************
    while ( len-- )
    {
	zp->re = zp->im = 0.0;
	zp++;
    }
    ******************************/
}


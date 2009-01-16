#include <../../nrnconf.h>
// This may look like C code, but it is really -*- C++ -*-
/* 
Copyright (C) 1988 Free Software Foundation
    written by Doug Lea (dl@rocky.oswego.edu)

This file is part of the GNU C++ Library.  This library is free
software; you can redistribute it and/or modify it under the terms of
the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.  This library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef __GNUG__
#pragma implementation
#endif
#include <ivstream.h>
#include <neuron_gnu_builtin.h>
#include "d_avec.h"

/*
 The following brought to you by the department of redundancy department
*/

doubleAVec& doubleAVec::operator = (doubleAVec& v)
{
  if (len != 0 && len != v.capacity())
    error("nonconformant vectors.");
  if (len == 0)
    s = new double [space = len = v.capacity()];
  if (s != v.vec())
  {
    for (int i = 0; i < len; ++i)
      s[i] = v.vec()[i];
  }
  return *this;
}

doubleAVec& doubleAVec::operator = (double  f)
{
  for (int i = 0; i < len; ++i) s[i] = f;
  return *this;
}


doubleAVec& concat(doubleAVec & a, doubleAVec & b)
{
  int newl = a.capacity() + b.capacity();
  doubleAVec* news = new doubleAVec(newl);
  double* p = news->s;
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  while (t < top) *p++ = *t++;
  top = &(b.vec()[b.capacity()]);
  t = b.vec();
  while (t < top) *p++ = *t++;
  return *news;
}


doubleAVec& combine(doubleCombiner f, doubleAVec& a, doubleAVec& b)
{
  int newl = (a.capacity() < b.capacity())? a.capacity() : b.capacity();
  doubleAVec* news = new doubleAVec(newl);
  double* p = news->s;
  double* top = &(a.vec()[newl]);
  double* t = a.vec();
  double* u = b.vec();
  while (t < top) *p++ = (*f)(*t++, *u++);
  return *news;
}

doubleAVec& reverse(doubleAVec& a)
{
  doubleAVec* news = new doubleAVec(a.capacity());
  if (a.capacity() != 0)
  {
    double* lo = news->s;
    double* hi = &(news->s[a.capacity() - 1]);
    while (lo < hi)
    {
      double tmp = *lo;
      *lo++ = *hi;
      *hi-- = tmp;
    }
  }
  return *news;
}

doubleAVec& mymap(doubleMapper f, doubleAVec& a)
{
  doubleAVec* news = new doubleAVec(a.capacity());
  double* p = news->s;
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  while(t < top) *p++ = (*f)(*t++);
  return *news;
}

doubleAVec& doubleAVec::at(int from, int n)
{
  int to;
  if (n < 0)
  {
    n = len - from;
    to = len - 1;
  }
  else
    to = from + n - 1;
  if ((unsigned)from > (unsigned)to)
    range_error();
  doubleAVec* news = new doubleAVec(n);
  double* p = news->s;
  double* t = &(s[from]);
  double* top = &(s[to]);
  while (t <= top) *p++ = *t++;
  return *news;
}

doubleAVec& merge(doubleAVec & a, doubleAVec & b, doubleComparator f)
{
  int newl = a.capacity() + b.capacity();
  doubleAVec* news = new doubleAVec(newl);
  double* p = news->s;
  double* topa = &(a.vec()[a.capacity()]);
  double* as = a.vec();
  double* topb = &(b.vec()[b.capacity()]);
  double* bs = b.vec();

  for (;;)
  {
    if (as >= topa)
    {
      while (bs < topb) *p++ = *bs++;
      break;
    }
    else if (bs >= topb)
    {
      while (as < topa) *p++ = *as++;
      break;
    }
    else if ((*f)(*as, *bs) <= 0)
      *p++ = *as++;
    else
      *p++ = *bs++;
  }
  return *news;
}

doubleAVec& operator + (doubleAVec& a, doubleAVec& b)
{
  a.check_len(b.capacity());
  doubleAVec* news = new doubleAVec(a.capacity());
  double* p = news->s;
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  double* u = b.vec();
  while (t < top) *p++ = *t++ + *u++;
  return *news;
}

doubleAVec& operator - (doubleAVec& a, doubleAVec& b)
{
  a.check_len(b.capacity());
  doubleAVec* news = new doubleAVec(a.capacity());
  double* p = news->s;
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  double* u = b.vec();
  while (t < top) *p++ = *t++ - *u++;
  return *news;
}

doubleAVec&  product(doubleAVec& a, doubleAVec& b)
{
  a.check_len(b.capacity());
  doubleAVec* news = new doubleAVec(a.capacity());
  double* p = news->s;
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  double* u = b.vec();
  while (t < top) *p++ = *t++ * *u++;
  return *news;
}

doubleAVec& quotient(doubleAVec& a, doubleAVec& b)
{
  a.check_len(b.capacity());
  doubleAVec* news = new doubleAVec(a.capacity());
  double* p = news->s;
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  double* u = b.vec();
  while (t < top) *p++ = *t++ / *u++;
  return *news;
}

doubleAVec& operator + (doubleAVec& a, double  b)
{
  doubleAVec* news = new doubleAVec(a.capacity());
  double* p = news->s;
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  while (t < top) *p++ = *t++ + b;
  return *news;
}

doubleAVec& operator - (doubleAVec& a, double  b)
{
  doubleAVec* news = new doubleAVec(a.capacity());
  double* p = news->s;
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  while (t < top) *p++ = *t++ - b;
  return *news;
}

doubleAVec& operator * (doubleAVec& a, double  b)
{
  doubleAVec* news = new doubleAVec(a.capacity());
  double* p = news->s;
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  while (t < top) *p++ = *t++ * b;
  return *news;
}

doubleAVec& operator / (doubleAVec& a, double  b)
{
  doubleAVec* news = new doubleAVec(a.capacity());
  double* p = news->s;
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  while (t < top) *p++ = *t++ / b;
  return *news;
}

doubleAVec& doubleAVec::operator - ()
{
  doubleAVec* news = new doubleAVec(len);
  double* p = news->s;
  double* top = &(s[len]);
  double* t = s;
  while (t < top) *p++ = -(*t++);
  return *news;
}

doubleAVec& doubleAVec::operator += (doubleAVec& b)
{
  check_len(b.capacity());
  double* u = b.vec();
  double* top = &(s[len]);
  double* t = s;
  while (t < top) *t++ += *u++;
  return *this;
}

doubleAVec& doubleAVec::operator -= (doubleAVec& b)
{
  check_len(b.capacity());
  double* u = b.vec();
  double* top = &(s[len]);
  double* t = s;
  while (t < top) *t++ -= *u++;
  return *this;
}

doubleAVec& doubleAVec::product(doubleAVec& b)
{
  check_len(b.capacity());
  double* u = b.vec();
  double* top = &(s[len]);
  double* t = s;
  while (t < top) *t++ *= *u++;
  return *this;
}

doubleAVec& doubleAVec::quotient(doubleAVec& b)
{
  check_len(b.capacity());
  double* u = b.vec();
  double* top = &(s[len]);
  double* t = s;
  while (t < top) *t++ /= *u++;
  return *this;
}

doubleAVec& doubleAVec::operator += (double  b)
{
  double* top = &(s[len]);
  double* t = s;
  while (t < top) *t++ += b;
  return *this;
}

doubleAVec& doubleAVec::operator -= (double  b)
{
  double* top = &(s[len]);
  double* t = s;
  while (t < top) *t++ -= b;
  return *this;
}

doubleAVec& doubleAVec::operator *= (double  b)
{
  double* top = &(s[len]);
  double* t = s;
  while (t < top) *t++ *= b;
  return *this;
}

doubleAVec& doubleAVec::operator /= (double  b)
{
  double* top = &(s[len]);
  double* t = s;
  while (t < top) *t++ /= b;
  return *this;
}

double doubleAVec::max()
{
  if (len == 0)
    return 0;
  double* top = &(s[len]);
  double* t = s;
  double res = *t++;
  for (; t < top; ++t) if (*t > res) res = *t;
  return res;
}

int doubleAVec::max_index()
{
  if (len == 0)
    return -1;
  int ind = 0;
  for (int i = 1; i < len; ++i)
    if (s[i] > s[ind])
      ind = i;
  return ind;
}

double doubleAVec::min()
{
  if (len == 0)
    return 0;
  double* top = &(s[len]);
  double* t = s;
  double res = *t++;
  for (; t < top; ++t) if (*t < res) res = *t;
  return res;
}

int doubleAVec::min_index()
{
  if (len == 0)
    return -1;
  int ind = 0;
  for (int i = 1; i < len; ++i)
    if (s[i] < s[ind])
      ind = i;
  return ind;
}

double doubleAVec::sum()
{
  double res = 0;
  double* top = &(s[len]);
  double* t = s;
  while (t < top) res += *t++;
  return res;
}


double doubleAVec::sumsq()
{
  double res = 0;
  double* top = &(s[len]);
  double* t = s;
  for (; t < top; ++t) res += *t * *t;
  return res;
}

double doubleAVec::mean()
{
  return sum()/len;
}

double doubleAVec::var()
{
  double res = 0;
  double* top = &(s[len]);
  double* t = s;
  double m = mean();
  for (; t < top; ++t) res += (*t - m) * (*t - m);
  return res/(len-1);
}

double doubleAVec::stdDev()
{
  return sqrt(var());
}

double operator * (doubleAVec& a, doubleAVec& b)
{
  a.check_len(b.capacity());
  double* top = &(a.vec()[a.capacity()]);
  double* t = a.vec();
  double* u = b.vec();
  double res = 0;
  while (t < top) res += *t++ * *u++;
  return res;
}

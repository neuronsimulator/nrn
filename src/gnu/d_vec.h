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


#ifndef _doubleVec_h
#ifdef __GNUG__
//#pragma interface
#endif
#define _doubleVec_h 1

#include <neuron_gnu_builtin.h>
#include "d_defs.h"

#ifndef _double_typedefs
#define _double_typedefs 1
typedef void (*doubleProcedure)(double );
typedef double  (*doubleMapper)(double );
typedef double  (*doubleCombiner)(double , double );
typedef int  (*doublePredicate)(double );
typedef int  (*doubleComparator)(double , double );
#endif


class doubleVec 
{
protected:      
  int                   len;
  int			space;	//allocated memory size
  double                   *s;                  

                        doubleVec(int l, double* d);
public:
                        doubleVec ();
                        doubleVec (int l);
                        doubleVec (int l, double  fill_value);
                        doubleVec (doubleVec&);
                        ~doubleVec ();

  doubleVec &              operator = (doubleVec & a);
  doubleVec&                at(int from = 0, int n = -1);

  int                   capacity();
  void                  resize(int newlen);                        

  double&                  operator [] (int n);
  double&                  elem(int n);

  friend doubleVec&         concat(doubleVec & a, doubleVec & b);
  friend doubleVec&         mymap(doubleMapper f, doubleVec & a);
  friend doubleVec&         merge(doubleVec & a, doubleVec & b, doubleComparator f);
  friend doubleVec&         combine(doubleCombiner f, doubleVec & a, doubleVec & b);
  friend doubleVec&         reverse(doubleVec & a);

  void                  reverse();
  void                  sort(doubleComparator f);
  void                  fill(double  val, int from = 0, int n = -1);

  void                  apply(doubleProcedure f);
  double                   reduce(doubleCombiner f, double  base);
  int                   index(double  targ);

  friend int            operator == (doubleVec& a, doubleVec& b);
  friend int            operator != (doubleVec& a, doubleVec& b);

  void                  error(const char* msg);
  void                  range_error();
};

extern void default_doubleVec_error_handler(const char*);
extern one_arg_error_handler_t doubleVec_error_handler;

extern one_arg_error_handler_t 
        set_doubleVec_error_handler(one_arg_error_handler_t f);


inline doubleVec::doubleVec()
{
  len = 0; s = 0; space = 0;
}

inline doubleVec::doubleVec(int l)
{
  s = new double [space = len = l];
}


inline doubleVec::doubleVec(int l, double* d) :len(l), space(l), s(d) {}


inline doubleVec::~doubleVec()
{
  delete [] s;
}


inline double& doubleVec::operator [] (int n)
{
  if ((unsigned)n >= (unsigned)len)
    range_error();
  return s[n];
}

inline double& doubleVec::elem(int n)
{
  return s[n];
}


inline int doubleVec::capacity()
{
  return len;
}


#ifndef MAC
inline int operator != (doubleVec& a, doubleVec& b)
{
  return !(a == b);
}
#endif

#endif

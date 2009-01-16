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


/* Modifed by ZFM 
   vec() made public
   var(), stdDev(), mean() added
*/

#ifndef _doubleAVec_h
#ifdef __GNUG__
//#pragma interface
#endif
#define _doubleAVec_h 1

#include "d_vec.h"

class doubleAVec : public doubleVec
{
protected:
  void                  check_len(int l);
                        doubleAVec(int l, double* d);
  public:
  double*               vec();
                        doubleAVec ();
                        doubleAVec (int l);
                        doubleAVec (int l, double  fill_value);
                        doubleAVec (doubleAVec&);
                        ~doubleAVec ();

  doubleAVec&              operator =  (doubleAVec& a);
  doubleAVec&              operator =  (double  fill_value);

// vector by scalar -> vector operations

  friend doubleAVec&        operator +  (doubleAVec& a, double  b);
  friend doubleAVec&        operator -  (doubleAVec& a, double  b);
  friend doubleAVec&        operator *  (doubleAVec& a, double  b);
  friend doubleAVec&        operator /  (doubleAVec& a, double  b);

  doubleAVec&              operator += (double  b);
  doubleAVec&              operator -= (double  b);
  doubleAVec&              operator *= (double  b);
  doubleAVec&              operator /= (double  b);

// vector by vector -> vector operations

  friend doubleAVec&        operator +  (doubleAVec& a, doubleAVec& b);
  friend doubleAVec&        operator -  (doubleAVec& a, doubleAVec& b);
  doubleAVec&              operator += (doubleAVec& b);
  doubleAVec&              operator -= (doubleAVec& b);

  doubleAVec&               operator - ();

  friend doubleAVec&        product(doubleAVec& a, doubleAVec& b);
  doubleAVec&              product(doubleAVec& b);
  friend doubleAVec&        quotient(doubleAVec& a, doubleAVec& b);
  doubleAVec&              quotient(doubleAVec& b);

// vector -> scalar operations

  friend double            operator * (doubleAVec& a, doubleAVec& b);
#undef min
#undef max
  double                   sum();
  double                   min();
  double                   max();
  double                   sumsq();
  double		   var();
  double		   mean();
  double  	  	   stdDev();

// indexing

  int                   min_index();
  int                   max_index();

// redundant but necesssary
  friend doubleAVec&        concat(doubleAVec& a, doubleAVec& b);
  friend doubleAVec&        mymap(doubleMapper f, doubleAVec& a);
  friend doubleAVec&        merge(doubleAVec& a, doubleAVec& b, doubleComparator f);
  friend doubleAVec&        combine(doubleCombiner f, doubleAVec& a, doubleAVec& b);
  friend doubleAVec&        reverse(doubleAVec& a);
  doubleAVec&               at(int from = 0, int n = -1);
};

inline doubleAVec::doubleAVec() {}
inline doubleAVec::doubleAVec(int l) :doubleVec(l) {}
inline doubleAVec::doubleAVec(int l, double  fill_value) : doubleVec (l, fill_value) {}
inline doubleAVec::doubleAVec(doubleAVec& v) :doubleVec(v) {}
inline doubleAVec::doubleAVec(int l, double* d) :doubleVec(l, d) {}
inline doubleAVec::~doubleAVec() {}


inline double* doubleAVec::vec()
{
  return s;
}


inline void doubleAVec::check_len(int l)
{
  if (l != len)
    error("nonconformant vectors.");
}

#endif

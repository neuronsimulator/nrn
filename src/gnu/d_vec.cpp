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
#include <stdlib.h>
#include <ivstream.h>
#include <neuron_gnu_builtin.h>
#include "d_vec.h"


//Machintosh definition for vector !=
#ifdef MAC
int operator != (doubleVec& a, doubleVec& b)
{
  return !(a == b);
}
#endif


// error handling


extern "C" {extern void hoc_execerror(const char*, const char*);}

void default_doubleVec_error_handler(const char* msg)
{
#if 1
	hoc_execerror("Vector error:", msg);
#else
  cerr << "Fatal doubleVec error. " << msg << "\n";
  exit(1);
#endif
}

one_arg_error_handler_t doubleVec_error_handler = default_doubleVec_error_handler;

one_arg_error_handler_t set_doubleVec_error_handler(one_arg_error_handler_t f)
{
  one_arg_error_handler_t old = doubleVec_error_handler;
  doubleVec_error_handler = f;
  return old;
}

void doubleVec::error(const char* msg)
{
  (*doubleVec_error_handler)(msg);
}

void doubleVec::range_error()
{
  (*doubleVec_error_handler)("Index out of range.");
}

doubleVec::doubleVec(doubleVec& v)
{
  s = new double [space = len = v.len];
  double* top = &(s[len]);
  double* t = s;
  double* u = v.s;
  while (t < top) *t++ = *u++;
}

doubleVec::doubleVec(int l, double  fill_value)
{
  s = new double [space = len = l];
  double* top = &(s[len]);
  double* t = s;
  while (t < top) *t++ = fill_value;
}


doubleVec& doubleVec::operator = (doubleVec& v)
{
  if (this != &v)
  {
    delete [] s;
    s = new double [space = len = v.len];
    double* top = &(s[len]);
    double* t = s;
    double* u = v.s;
    while (t < top) *t++ = *u++;
  }
  return *this;
}

void doubleVec::apply(doubleProcedure f)
{
  double* top = &(s[len]);
  double* t = s;
  while (t < top) (*f)(*t++);
}

#if 0

As you mentioned, resizing vectors is an expensive process involving
allocate new space, copy, release old space. Of course this is entirely
appropriate for most usage but not when vectors are used to represent streams
where the size repeatedly runs from 0 to some indeterminate number.
In this case we want them to be able to grow and shrink efficiently.

My proposal is to manage storage a bit differently. The result will be that
vectors behave exactly as now (capacity() equals the protected variable len)
but the memory allocation is decoupled from this number and this space
tends to grow in moderate sized chunks but never gets released until the
entire vector object is destructed.
This means that a resize merely changes len as long as len < space.

#endif

// can't just realloc since there may be need for constructors/destructors
void doubleVec::resize(int newl)
{
 if (newl > space) {
  double* news = new double [newl];
  double* p = news;
  int minl = (len < newl)? len : newl;
  double* top = &(s[minl]);
  double* t = s;
  while (t < top) *p++ = *t++;
  delete [] s;
  s = news;
  space = newl;
 }
  len = newl;
}

doubleVec& concat(doubleVec & a, doubleVec & b)
{
  int newl = a.len + b.len;
  doubleVec* news = new doubleVec(newl);
  double* p = news->s;
  double* top = &(a.s[a.len]);
  double* t = a.s;
  while (t < top) *p++ = *t++;
  top = &(b.s[b.len]);
  t = b.s;
  while (t < top) *p++ = *t++;
  return *news;
}


doubleVec& combine(doubleCombiner f, doubleVec& a, doubleVec& b)
{
  int newl = (a.len < b.len)? a.len : b.len;
  doubleVec* news = new doubleVec(newl);
  double* p = news->s;
  double* top = &(a.s[newl]);
  double* t = a.s;
  double* u = b.s;
  while (t < top) *p++ = (*f)(*t++, *u++);
  return *news;
}

double doubleVec::reduce(doubleCombiner f, double  base)
{
  double r = base;
  double* top = &(s[len]);
  double* t = s;
  while (t < top) r = (*f)(r, *t++);
  return r;
}

doubleVec& reverse(doubleVec& a)
{
  doubleVec* news = new doubleVec(a.len);
  if (a.len != 0)
  {
    double* lo = news->s;
    double* hi = &(news->s[a.len - 1]);
    while (lo < hi)
    {
      double tmp = *lo;
      *lo++ = *hi;
      *hi-- = tmp;
    }
  }
  return *news;
}

void doubleVec::reverse()
{
  if (len != 0)
  {
    double* lo = s;
    double* hi = &(s[len - 1]);
    while (lo < hi)
    {
      double tmp = *lo;
      *lo++ = *hi;
      *hi-- = tmp;
    }
  }
}

int doubleVec::index(double  targ)
{
  for (int i = 0; i < len; ++i) if (doubleEQ(targ, s[i])) return i;
  return -1;
}

doubleVec& mymap(doubleMapper f, doubleVec& a)
{
  doubleVec* news = new doubleVec(a.len);
  double* p = news->s;
  double* top = &(a.s[a.len]);
  double* t = a.s;
  while(t < top) *p++ = (*f)(*t++);
  return *news;
}

int operator == (doubleVec& a, doubleVec& b)
{
  if (a.len != b.len)
    return 0;
  double* top = &(a.s[a.len]);
  double* t = a.s;
  double* u = b.s;
  while (t < top) if (!(doubleEQ(*t++, *u++))) return 0;
  return 1;
}

void doubleVec::fill(double  val, int from, int n)
{
  int to;
  if (n < 0)
    to = len - 1;
  else
    to = from + n - 1;
  if ((unsigned)from > (unsigned)to)
    range_error();
  double* t = &(s[from]);
  double* top = &(s[to]);
  while (t <= top) *t++ = val;
}

doubleVec& doubleVec::at(int from, int n)
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
  doubleVec* news = new doubleVec(n);
  double* p = news->s;
  double* t = &(s[from]);
  double* top = &(s[to]);
  while (t <= top) *p++ = *t++;
  return *news;
}

doubleVec& merge(doubleVec & a, doubleVec & b, doubleComparator f)
{
  int newl = a.len + b.len;
  doubleVec* news = new doubleVec(newl);
  double* p = news->s;
  double* topa = &(a.s[a.len]);
  double* as = a.s;
  double* topb = &(b.s[b.len]);
  double* bs = b.s;

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

static int gsort(double*, int, doubleComparator);
 
void doubleVec::sort (doubleComparator compar)
{
  gsort(s, len, compar);
}


// An adaptation of Schmidt's new quicksort

static inline void SWAP(double* A, double* B)
{
  double tmp = *A; *A = *B; *B = tmp;
}

/* This should be replaced by a standard ANSI macro. */
#define BYTES_PER_WORD 8
#define BYTES_PER_LONG 4

/* The next 4 #defines implement a very fast in-line stack abstraction. */

#define STACK_SIZE (BYTES_PER_WORD * BYTES_PER_LONG)
#define PUSH(LOW,HIGH) do {top->lo = LOW;top++->hi = HIGH;} while (0)
#define POP(LOW,HIGH)  do {LOW = (--top)->lo;HIGH = top->hi;} while (0)
#define STACK_NOT_EMPTY (stack < top)                

/* Discontinue quicksort algorithm when partition gets below this size.
   This particular magic number was chosen to work best on a Sun 4/260. */
#define MAX_THRESH 4


/* Order size using quicksort.  This implementation incorporates
   four optimizations discussed in Sedgewick:
   
   1. Non-recursive, using an explicit stack of pointer that
      store the next array partition to sort.  To save time, this
      maximum amount of space required to store an array of
      MAX_INT is allocated on the stack.  Assuming a 32-bit integer,
      this needs only 32 * sizeof (stack_node) == 136 bits.  Pretty
      cheap, actually.

   2. Chose the pivot element using a median-of-three decision tree.
      This reduces the probability of selecting a bad pivot value and 
      eliminates certain extraneous comparisons.

   3. Only quicksorts TOTAL_ELEMS / MAX_THRESH partitions, leaving
      insertion sort to order the MAX_THRESH items within each partition.  
      This is a big win, since insertion sort is faster for small, mostly
      sorted array segements.
   
   4. The larger of the two sub-partitions is always pushed onto the
      stack first, with the algorithm then concentrating on the
      smaller partition.  This *guarantees* no more than log (n)
      stack size is needed! */
      
static int gsort (double *base_ptr, int total_elems, doubleComparator cmp)
{
/* Stack node declarations used to store unfulfilled partition obligations. */
  struct stack_node {  double *lo;  double *hi; };
  double   pivot_buffer;
  int   max_thresh   = MAX_THRESH;

  if (total_elems > MAX_THRESH)
    {
      double       *lo = base_ptr;
      double       *hi = lo + (total_elems - 1);
      double       *left_ptr;
      double       *right_ptr;
      stack_node stack[STACK_SIZE]; /* Largest size needed for 32-bit int!!! */
      stack_node *top = stack + 1;

      while (STACK_NOT_EMPTY)
        {
          {
            double *pivot = &pivot_buffer;
            {
              /* Select median value from among LO, MID, and HI. Rearrange
                 LO and HI so the three values are sorted. This lowers the 
                 probability of picking a pathological pivot value and 
                 skips a comparison for both the LEFT_PTR and RIGHT_PTR. */

              double *mid = lo + ((hi - lo) >> 1);

              if ((*cmp) (*mid, *lo) < 0)
                SWAP (mid, lo);
              if ((*cmp) (*hi, *mid) < 0)
              {
                SWAP (mid, hi);
                if ((*cmp) (*mid, *lo) < 0)
                  SWAP (mid, lo);
              }
              *pivot = *mid;
              pivot = &pivot_buffer;
            }
            left_ptr  = lo + 1;
            right_ptr = hi - 1; 

            /* Here's the famous ``collapse the walls'' section of quicksort.  
               Gotta like those tight inner loops!  They are the main reason 
               that this algorithm runs much faster than others. */
            do 
              {
                while ((*cmp) (*left_ptr, *pivot) < 0)
                  left_ptr += 1;

                while ((*cmp) (*pivot, *right_ptr) < 0)
                  right_ptr -= 1;

                if (left_ptr < right_ptr) 
                  {
                    SWAP (left_ptr, right_ptr);
                    left_ptr += 1;
                    right_ptr -= 1;
                  }
                else if (left_ptr == right_ptr) 
                  {
                    left_ptr += 1;
                    right_ptr -= 1;
                    break;
                  }
              } 
            while (left_ptr <= right_ptr);

          }

          /* Set up pointers for next iteration.  First determine whether
             left and right partitions are below the threshold size. If so, 
             ignore one or both.  Otherwise, push the larger partition's
             bounds on the stack and continue sorting the smaller one. */

          if ((right_ptr - lo) <= max_thresh)
            {
              if ((hi - left_ptr) <= max_thresh) /* Ignore both small partitions. */
                POP (lo, hi); 
              else              /* Ignore small left partition. */  
                lo = left_ptr;
            }
          else if ((hi - left_ptr) <= max_thresh) /* Ignore small right partition. */
            hi = right_ptr;
          else if ((right_ptr - lo) > (hi - left_ptr)) /* Push larger left partition indices. */
            {                   
              PUSH (lo, right_ptr);
              lo = left_ptr;
            }
          else                  /* Push larger right partition indices. */
            {                   
              PUSH (left_ptr, hi);
              hi = right_ptr;
            }
        }
    }

  /* Once the BASE_PTR array is partially sorted by quicksort the rest
     is completely sorted using insertion sort, since this is efficient 
     for partitions below MAX_THRESH size. BASE_PTR points to the beginning 
     of the array to sort, and END_PTR points at the very last element in
     the array (*not* one beyond it!). */


  {
    double *end_ptr = base_ptr + 1 * (total_elems - 1);
    double *run_ptr;
    double *tmp_ptr = base_ptr;
    double *thresh  = (end_ptr < (base_ptr + max_thresh))? 
      end_ptr : (base_ptr + max_thresh);

    /* Find smallest element in first threshold and place it at the
       array's beginning.  This is the smallest array element,
       and the operation speeds up insertion sort's inner loop. */

    for (run_ptr = tmp_ptr + 1; run_ptr <= thresh; run_ptr += 1)
      if ((*cmp) (*run_ptr, *tmp_ptr) < 0)
        tmp_ptr = run_ptr;

    if (tmp_ptr != base_ptr)
      SWAP (tmp_ptr, base_ptr);

    /* Insertion sort, running from left-hand-side up to `right-hand-side.' 
       Pretty much straight out of the original GNU qsort routine. */

    for (run_ptr = base_ptr + 1; (tmp_ptr = run_ptr += 1) <= end_ptr; )
      {

        while ((*cmp) (*run_ptr, *(tmp_ptr -= 1)) < 0)
          ;

        if ((tmp_ptr += 1) != run_ptr)
          {
            double *trav;

            for (trav = run_ptr + 1; --trav >= run_ptr;)
              {
                double c = *trav;
                double *hi, *lo;

                for (hi = lo = trav; (lo -= 1) >= tmp_ptr; hi = lo)
                  *hi = *lo;
                *hi = c;
              }
          }

      }
  }
  return 1;
}

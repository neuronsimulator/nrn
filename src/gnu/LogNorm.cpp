#include <../../nrnconf.h>
/* 
Copyright (C) 1988 Free Software Foundation
    written by Dirk Grunwald (grunwald@cs.uiuc.edu)

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
#include <Random.h>
#include <Normal.h>

#include <LogNorm.h>

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

//
//	See Simulation, Modelling & Analysis by Law & Kelton, pp260
//
// 

double LogNormal::operator()()
{
    return( pow(M_E, this->Normal::operator()() ) );
}



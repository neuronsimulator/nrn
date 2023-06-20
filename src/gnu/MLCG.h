// This may look like C code, but it is really -*- C++ -*-
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
#ifndef _MLCG_h
#define _MLCG_h 1 
#ifdef __GNUG__
//#pragma interface
#endif

#include "RNG.h"
#include <math.h>

//
//	Multiplicative Linear Conguential Generator
//

class MLCG : public RNG {
    int32_t initialSeedOne;
    int32_t initialSeedTwo;
    int32_t seedOne;
    int32_t seedTwo;

protected:

public:
    MLCG(int32_t seed1 = 0, int32_t seed2 = 1);
    //
    // Return a long-words word of random bits
    //
    virtual uint32_t asLong();
    virtual void reset();
    int32_t seed1();
    void seed1(int32_t);
    int32_t seed2();
    void seed2(int32_t);
    void reseed(int32_t, int32_t);
};

inline int32_t
MLCG::seed1()
{
    return(seedOne);
}

inline void
MLCG::seed1(int32_t s)
{
    initialSeedOne = s;
    reset();
}

inline int32_t
MLCG::seed2()
{
    return(seedTwo);
}

inline void
MLCG::seed2(int32_t s)
{
    initialSeedTwo = s;
    reset();
}

inline void
MLCG::reseed(int32_t s1, int32_t s2)
{
    initialSeedOne = s1;
    initialSeedTwo = s2;
    reset();
}

#endif

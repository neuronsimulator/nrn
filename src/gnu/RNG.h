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
#pragma once
/*
 * int32_t and uint32_t have been defined by the configure procedure.  Just
 * use these in place of the ones that libg++ used to provide. 
 */
#include <cstdint>
#include <cassert>
#include <cmath>
#include <limits>

union PrivateRNGSingleType {		   	// used to access floats as unsigneds
    float s;
    uint32_t u;
};

union PrivateRNGDoubleType {		   	// used to access doubles as unsigneds
    double d;
    uint32_t u[2];
};

//
// Base class for Random Number Generators.
//
class RNG {
    static PrivateRNGSingleType singleMantissa;	// mantissa bit vector
    static PrivateRNGDoubleType doubleMantissa;	// mantissa bit vector
public:
    using result_type = std::uint32_t;

    RNG();
    virtual ~RNG();
    //
    // Return a long-words word of random bits
    //
    virtual result_type asLong() = 0;
    virtual void reset() = 0;
    //
    // Return random bits converted to either a float or a double
    //
    virtual float asFloat();
    virtual double asDouble();

    static constexpr result_type min() {
        return std::numeric_limits<result_type>::min();
    }
    static constexpr result_type max() {
        return std::numeric_limits<result_type>::max();
    }
    result_type operator()() {
        return asLong();
    }
};

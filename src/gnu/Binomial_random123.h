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

#include <random>

#include <Random.h>
#include <RNG_random123.h>

class Binomial_random123: public Random_random123 {
protected:
    std::binomial_distribution<> d;
public:
    Binomial_random123(int t, double p, RNG_random123 *gen);

    int t();
    int t(int xt);

    double p();
    double p(int xp);

    virtual double operator()();

};


inline Binomial_random123::Binomial_random123(int t, double p, RNG_random123 *gen)
: Random_random123(gen) {
  d = std::binomial_distribution<>(t, p);
}

inline int Binomial_random123::t() { return d.t(); }
inline int Binomial_random123::t(int xt) { int tmp = d.t(); d.param(std::binomial_distribution<>::param_type(xt, d.p())); return tmp; }

inline double Binomial_random123::p() { return d.p(); }
inline double Binomial_random123::p(int xp) { double tmp = d.p(); d.param(std::binomial_distribution<>::param_type(d.t(), xp)); return tmp; }

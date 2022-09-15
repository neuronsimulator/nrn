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
#ifndef _Random_h
#define _Random_h 1
#ifdef __GNUG__
//#pragma interface
#endif

#include <math.h>

#if MAC
#define Random gnu_Random
#endif

#include <RNG.h>
#include <RNG_random123.h>

class Random {
  protected:
    RNG* pGenerator;

  public:
    Random(RNG* generator);
    virtual ~Random() {}
    virtual double operator()() = 0;

    RNG* generator();
    void generator(RNG* p);
};


inline Random::Random(RNG *gen)
{
    pGenerator = gen;
}

inline RNG *Random::generator()
{
    return(pGenerator);
}

inline void Random::generator(RNG *p)
{
    pGenerator = p;
}

class Random_random123 {
  protected:
    RNG_random123* pGenerator;

  public:
    Random_random123(RNG_random123* generator);
    virtual ~Random_random123() {}
    virtual double operator()() = 0;

    RNG_random123* generator();
    void generator(RNG_random123* p);
};


inline Random_random123::Random_random123(RNG_random123* gen) {
    pGenerator = gen;
}

inline RNG_random123* Random_random123::generator() {
    return (pGenerator);
}

inline void Random_random123::generator(RNG_random123* p) {
    pGenerator = p;
}

#include <random>
class Binomial_random123: public Random_random123 {
  protected:
    std::binomial_distribution<> d;

  public:
    inline Binomial_random123(int t, double p, RNG_random123* gen)
        : Random_random123(gen)
        , d(t, p) {}

    inline double operator()() override {
        return (d(*(generator()->get_random_generator())));
    }
};

class DiscreteUniform_random123: public Random_random123 {
  protected:
    std::uniform_int_distribution<> d;

  public:
    inline DiscreteUniform_random123(double low, double high, RNG_random123* gen)
        : Random_random123(gen)
        , d(low, high) {}

    inline double operator()() override {
        return (d(*(generator()->get_random_generator())));
    }
};

class Erlang_random123: public Random_random123 {
  protected:
    double pMean;
    double pVariance;
    int a;
    double b;
    inline void setState() {
        a = int((pMean * pMean) / pVariance + 0.5);
        a = (a > 0) ? a : 1;
        b = pMean / a;
    }

    std::gamma_distribution<> d;

  public:
    inline Erlang_random123(double mean, double variance, RNG_random123* gen)
        : Random_random123(gen)
        , pMean(mean)
        , pVariance(variance) {
        setState();
        d = std::gamma_distribution<>(a, b);
    }

    inline double operator()() override {
        return (d(*(generator()->get_random_generator())));
    }
};

class Geometric_random123: public Random_random123 {
  protected:
    std::geometric_distribution<> d;

  public:
    Geometric_random123(double mean, RNG_random123* gen)
        : Random_random123(gen)
        , d(mean) {}

    double operator()() override {
        return (1. + d(*(generator()->get_random_generator())));
    }
};

class Normal_random123: public Random_random123 {
  protected:
    std::normal_distribution<> d;

  public:
    inline Normal_random123(double mean, double variance, RNG_random123* gen)
        : Random_random123(gen)
        , d(mean, std::sqrt(variance)) {}

    inline double operator()() override {
        return (d(*(generator()->get_random_generator())));
    }
};

class LogNormal_random123: public Normal_random123 {
  public:
    inline LogNormal_random123(double mean, double variance, RNG_random123* gen)
        : Normal_random123(std::log(mean * mean / std::sqrt(variance + (mean * mean))),
                           log((variance + (mean * mean)) / (mean * mean)),
                           gen) {
    }

    double operator()() override {
        return (std::exp(this->Normal_random123::operator()()));
    }
};

class NegativeExpntl_random123: public Random_random123 {
  protected:
    std::exponential_distribution<> d;

  public:
    inline NegativeExpntl_random123(double xmean, RNG_random123* gen)
        : Random_random123(gen)
        , d(1 / xmean) {}

    inline double operator()() override {
        return (d(*(generator()->get_random_generator())));
    }
};

class Poisson_random123: public Random_random123 {
  protected:
    std::poisson_distribution<> d;

  public:
    inline Poisson_random123(double mean, RNG_random123* gen)
        : Random_random123(gen)
        , d(mean) {}

    inline double operator()() override {
        return (d(*(generator()->get_random_generator())));
    }
};

class Uniform_random123: public Random_random123 {
  protected:
    std::uniform_real_distribution<> d;

  public:
    inline Uniform_random123(double low, double high, RNG_random123* gen)
        : Random_random123(gen)
        , d(low, high) {}

    inline double operator()() override {
        return (d(*(generator()->get_random_generator())));
    }
};

class Weibull_random123: public Random_random123 {
  protected:
    std::weibull_distribution<> d;

  public:
    inline Weibull_random123(double alpha, double beta, RNG_random123* gen)
        : Random_random123(gen)
        , d(alpha, beta) {}

    inline double operator()() override {
        return (d(*(generator()->get_random_generator())));
    }
};
#endif

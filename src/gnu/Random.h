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

    inline int t() {
        return d.t();
    }

    inline int t(int xt) {
        int tmp = d.t();
        d.param(std::binomial_distribution<>::param_type(xt, d.p()));
        return tmp;
    }

    inline double p() {
        return d.p();
    }

    inline double p(int xp) {
        double tmp = d.p();
        d.param(std::binomial_distribution<>::param_type(d.t(), xp));
        return tmp;
    }

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

    inline double low() {
        return d.min();
    }

    inline double low(double x) {
        double tmp = d.min();
        d.param(std::uniform_int_distribution<>::param_type(x, d.max()));
        return tmp;
    }

    inline double high() {
        return d.max();
    }

    inline double high(double x) {
        double tmp = d.max();
        d.param(std::uniform_int_distribution<>::param_type(d.min(), x));
        return tmp;
    }

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

    inline double mean() {
        return pMean;
    }

    inline double mean(double x) {
        double tmp = pMean;
        pMean = x;
        setState();
        d.param(std::gamma_distribution<>::param_type(a, b));
        return tmp;
    };

    inline double variance() {
        return pVariance;
    }

    inline double variance(double x) {
        double tmp = pVariance;
        pVariance = x;
        setState();
        d.param(std::gamma_distribution<>::param_type(a, b));
        return tmp;
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

    inline double mean() {
        return d.p();
    }

    inline double mean(double x) {
        double tmp = d.p();
        d.param(std::geometric_distribution<>::param_type(x));
        return tmp;
    }

    double operator()() override {
        return (1. + d(*(generator()->get_random_generator())));
    }
};

class Normal_random123: public Random_random123 {
  protected:
    std::normal_distribution<> d;
    double pMean;
    double pVariance;
    double pStdDev;

  public:
    inline Normal_random123(double mean, double variance, RNG_random123* gen)
        : Random_random123(gen)
        , d(mean, std::sqrt(variance))
        , pMean(mean)
        , pVariance(variance)
        , pStdDev(std::sqrt(variance)) {}

    inline double mean() {
        return d.mean();
    }

    inline double mean(double x) {
        pMean = x;
        double t = d.mean();
        d.param(std::normal_distribution<>::param_type(x, d.stddev()));
        return t;
    }

    inline double variance() {
        return d.stddev() * d.stddev();
    }

    inline double variance(double x) {
        pStdDev = std::sqrt(x);
        pVariance = x;
        double t = d.stddev() * d.stddev();
        d.param(std::normal_distribution<>::param_type(d.mean(), pStdDev));
        return t;
    }

    inline double operator()() override {
        return (d(*(generator()->get_random_generator())));
    }
};

class LogNormal_random123: public Normal_random123 {
  protected:
    double logMean;
    double logVariance;
    inline void setState() {
        double m2 = logMean * logMean;
        pMean = log(m2 / sqrt(logVariance + m2));
        // from ch@heike.informatik.uni-dortmund.de:
        // (was   pVariance = log((sqrt(logVariance + m2)/m2 )); )
        pStdDev = sqrt(log((logVariance + m2) / m2));
    }

  public:
    inline LogNormal_random123(double mean, double variance, RNG_random123* gen)
        : Normal_random123(std::log(mean * mean / std::sqrt(variance + (mean * mean))),
                           log((variance + (mean * mean)) / (mean * mean)),
                           gen) {
        logMean = mean;
        logVariance = variance;
        setState();
    }

    inline double mean() {
        return logMean;
    }

    inline double mean(double x) {
        double t = logMean;
        logMean = x;
        setState();
        Normal_random123::mean(pMean);
        return t;
    }

    inline double variance() {
        return logVariance;
    }

    inline double variance(double x) {
        double t = logVariance;
        logVariance = x;
        setState();
        Normal_random123::variance(std::sqrt(pStdDev));
        return t;
    }

    double operator()() override {
        return (std::exp(this->Normal_random123::operator()()));
    }
};

class NegativeExpntl_random123: public Random_random123 {
  protected:
    double pMean;
    double lambda;
    std::exponential_distribution<> d;

  public:
    inline NegativeExpntl_random123(double xmean, RNG_random123* gen)
        : Random_random123(gen)
        , pMean(xmean)
        , lambda(1 / pMean)
        , d(lambda) {}

    inline double mean() {
        return pMean;
    }
    inline double mean(double x) {
        double t = pMean;
        pMean = x;
        lambda = 1 / pMean;
        d.param(std::exponential_distribution<>::param_type(lambda));
        return t;
    }

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

    inline double mean() {
        return d.mean();
    }

    inline double mean(double x) {
        double t = d.mean();
        d.param(std::poisson_distribution<>::param_type(x));
        return t;
    }

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

    inline double low() {
        return d.min();
    }

    inline double low(double x) {
        double tmp = d.min();
        d.param(std::uniform_real_distribution<>::param_type(x, d.max()));
        return tmp;
    }

    inline double high() {
        return d.max();
    }

    inline double high(double x) {
        double tmp = d.max();
        d.param(std::uniform_real_distribution<>::param_type(d.min(), x));
        return tmp;
    }

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

    inline double alpha() {
        return d.a();
    }

    inline double alpha(double x) {
        double tmp = d.a();
        d.param(std::weibull_distribution<>::param_type(x, d.b()));
        return tmp;
    }

    inline double beta() {
        return d.b();
    }

    inline double beta(double x) {
        double tmp = d.b();
        d.param(std::weibull_distribution<>::param_type(d.a(), x));
        return tmp;
    }

    inline double operator()() override {
        return (d(*(generator()->get_random_generator())));
    }
};
#endif

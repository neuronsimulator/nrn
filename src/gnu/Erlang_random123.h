#pragma once

#include <random>

#include <Random.h>
#include <RNG_random123.h>

class Erlang_random123: public Random_random123 {
protected:
    double pMean;
    double pVariance;
    int a;
    double b;
    void setState();
    std::gamma_distribution<> d;
public:
    Erlang_random123(double mean, double variance, RNG_random123 *gen);

    double mean();
    double mean(double x);
    double variance();
    double variance(double x);

    virtual double operator()();

};


inline void Erlang_random123::setState() {
  a = int( (pMean * pMean ) / pVariance + 0.5 );
  a = (a > 0) ? a : 1;
  b = pMean / a;
}

inline Erlang_random123::Erlang_random123(double mean, double variance, RNG_random123 *gen) : Random_random123(gen)
{
  setState();
  pMean = mean;
  pVariance = variance;
  d = std::gamma_distribution<>(a, b);
}

inline double Erlang_random123::mean() {
  return pMean;
}

inline double Erlang_random123::mean(double x) {
  double tmp = pMean;
  pMean = x;
  setState();
  d.param(std::gamma_distribution<>::param_type(a, b));
  return tmp;
};

inline double Erlang_random123::variance() {
  return pVariance;
}

inline double Erlang_random123::variance(double x) {
  double tmp = pVariance;
  pVariance = x;
  setState();
  d.param(std::gamma_distribution<>::param_type(a, b));
  return tmp;
}

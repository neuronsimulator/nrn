#pragma once

#include <random>

#include <Random.h>
#include <RNG_random123.h>

class Geometric_random123: public Random_random123 {
protected:
    std::geometric_distribution<> d;
public:
    Geometric_random123(double mean, RNG_random123 *gen);

    double mean();
    double mean(double x);

    virtual double operator()();

};

inline Geometric_random123::Geometric_random123(double p, RNG_random123 *gen) : Random_random123(gen)
{
  d = std::geometric_distribution<>(p);
}

inline double Geometric_random123::mean() {
  return d.p();
}

inline double Geometric_random123::mean(double x) {
  double tmp = d.p();
  d.param(std::geometric_distribution<>::param_type(x));
  return tmp;
}
